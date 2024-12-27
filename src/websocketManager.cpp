
#include "websocketManager.h"
#include "BS_thread_pool.hpp"
#include "packets.h"
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <mutex>
#include <poll.h>
#include <queue>
#include <sched.h>
#include <shared_mutex>
#include <string_view>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <tuple>
#include <unistd.h>
#include <vector>

WebSocketManager::WebSocketManager(WebSocket *ws, size_t id,
                                   SystemWatcher *watcher,
                                   SystemInterface *interface,
                                   BS::thread_pool<BS::tp::none> *threadpool)
    : ws(ws), parent(watcher), interface(interface), socket_id(id),
      threadpool(threadpool) {
  auto user_data = ws->getUserData();
  user_data->id = id;
  user_data->manager = this;

  // initial continue
  send_continue(BUFFER_COUNT, 0);

  std::lock_guard gaurd(parent->watched_sockets_lock);
  parent->watched_sockets[id] = this;
}
void WebSocketManager::force_close() {
  has_backpressure = false;
  has_backpressure.notify_all();
#ifdef DEBUG
  printf("force closing\n");
#endif
  {
    std::vector<uint32_t> ids;
    {
      std::unique_lock<std::shared_mutex> stream_gaurd(stream_lock);
      for (auto stream : streams) {
        ids.push_back(stream.first);
      }
    }
    for (auto id : ids) {
      close_stream(id, true);
    }
  }
}

WebSocketManager::~WebSocketManager() {
  std::lock_guard gaurd(parent->watched_sockets_lock);
  parent->watched_sockets.erase(socket_id);
}
void WebSocketManager::close_stream(uint32_t stream_id, bool remove_poll) {
  std::unique_lock<std::shared_mutex> stream_gaurd(stream_lock);

  send_close(0x02, stream_id);

  if (remove_poll)
    parent->epoll.erase_fd(streams[stream_id].fd);

#ifdef PCAP
  delete streams[stream_id].pcap;
#endif

  close(streams[stream_id].fd);
  streams.erase(stream_id);
#ifdef DEBUG
  printf("closed stream\n");
#endif
}

std::optional<int> WebSocketManager::handle_connect(WispPacket packet) {
  uint32_t stream_id = packet.stream_id;
  ConnectPacket connect_packet(packet.data.get(), packet.data_len);
  uint8_t stream_type = connect_packet.stream_type;
#ifdef DEBUG
  printf("got connect %s\n", connect_packet.destination_hostname.get());
#endif

  auto connection =
      interface->open_stream((char *)connect_packet.destination_hostname.get(),
                             stream_type, connect_packet.destination_port);
  if (!connection.has_value()) {
    return {};
  }

  SocketStreamData stream_data = {.stream_type = stream_type,
                                  .id = stream_id,
                                  .fd = connection->first,
                                  .fd_info = connection->second};
#ifdef PCAP
  stream_data.open_pcap();
#endif

  parent->epoll.push_fd(connection->first);

  // TODO: log connection
  std::unique_lock<std::shared_mutex> gaurd(stream_lock);
  streams[stream_id] = stream_data;

  parent->epoll.wake();
  return stream_id;
}

std::optional<uint32_t> WebSocketManager::handle_data(WispPacket packet) {
  std::shared_lock<std::shared_mutex> gaurd(stream_lock);
  //  if stream isnt real
  if (streams.find(packet.stream_id) == streams.end())
    return {};

  std::future<void> res = threadpool->submit_task([this, packet]() {
    std::shared_lock<std::shared_mutex> gaurd(stream_lock);

    int err = EAGAIN;
    while (err == EAGAIN) {
      err = 0;
      struct pollfd pfd{.fd = streams.find(packet.stream_id)->second.fd,
                        .events = POLLOUT};
#ifdef PCAP
      streams.find(packet.stream_id)
          ->second.pcap->write_dummy_user(packet.data.get(), packet.data_len);
#endif
      int res = poll(&pfd, 1, -1);
      if (res != -1) {
        if (write(streams.find(packet.stream_id)->second.fd,
                  (void *)packet.data.get(), packet.data_len) == -1)
          err = errno;
      }
    }
  });

  if (streams[packet.stream_id].stream_type != 0x02) {
    streams[packet.stream_id].buffer--;

    // reset message buffer
    if (streams[packet.stream_id].buffer == 0 && has_backpressure &&
        parent->send_queue_len < MAX_QUEUE) {
      streams[packet.stream_id].buffer = BUFFER_COUNT;
      send_continue(BUFFER_COUNT, packet.stream_id);
    }
  }

  return streams[packet.stream_id].buffer;
}
void SystemWatcher::watch() {
  while (true) {
    // make killable
    if (deconstructing) {
      break;
    }

    epoll_event events[BUFFER_COUNT];
    auto event_count = epoll.wait(events);
    if (!event_count.has_value()) {
      continue;
    }
    for (int i = 0; i < event_count; i++) {
      if (events[i].data.fd == epoll.pipes[0]) {
#ifdef DEBUG
        printf("woken\n");
#endif
        char buf[10];
        read(events[i].data.fd, buf, 10);
        continue;
      } else {

        int fd = events[i].data.fd;

        ssize_t count;
        bool to_close = false;
        std::vector<char> *combined_buffer = new std::vector<char>();
        do {
          char buf[SEGMENT_SIZE];

          count = read(events[i].data.fd, buf, SEGMENT_SIZE);

          if (count == 0) {
            to_close = true;
            break;
          }
          if (count < 0) {
            if (errno == EAGAIN) {
              break;
            }
            to_close = true;
            break;
          }

          combined_buffer->insert(combined_buffer->end(), buf, buf + count);
        } while (true);

        // we cant wait on the other thread to close the stream
        if (to_close) {
          epoll.erase_fd(events[i].data.fd);
        }

        send_queue_len++;

        // this mess finds the socket and string to send it
        loop->defer([this, combined_buffer, to_close, fd]() {
          std::optional<uint32_t> found_id;
          std::optional<WebSocketManager *> found_manager;
          std::lock_guard sockets_lock(watched_sockets_lock);
          for (auto socket_manager : watched_sockets) {
            std::shared_lock<std::shared_mutex> stream_lock(
                socket_manager.second->stream_lock);

            for (auto stream : socket_manager.second->streams) {
              if (stream.second.fd == fd) {

                if (combined_buffer->size() > MAX_WS_FRAME) {
#ifdef DEBUG
                  printf("sent data is too big %zu\n", combined_buffer->size());
#endif
                }

                auto ret = socket_manager.second->send_data(
                    stream.first, combined_buffer->data(),
                    combined_buffer->size());
                if (ret == WebSocket::SendStatus::BACKPRESSURE) {
#ifdef DEBUG
                  printf("has backpressure\n");
#endif
                  socket_manager.second->has_backpressure = true;
                }
                if (ret == WebSocket::SendStatus::DROPPED) {
#ifdef DEBUG
                  printf("dropped\n");
#endif
                }

                found_id = stream.first;
                found_manager = socket_manager.second;
              }
            }
          }
          if (to_close && found_id.has_value() && found_manager.has_value()) {
            found_manager.value()->close_stream(found_id.value(), false);
          }

          send_queue_len--;
          if (send_queue_len < MAX_QUEUE) {
            for (auto socket_manager : watched_sockets) {
              socket_manager.second->update_streams();
            }
          }
          delete combined_buffer;
        });
      }
    }
  }
}
