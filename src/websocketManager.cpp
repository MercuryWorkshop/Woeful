
#include "websocketManager.hpp"
#include "packets.h"
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
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
                                   SystemInterface *interface)
    : ws(ws), parent(watcher), interface(interface), socket_id(id) {
  auto user_data = ws->getUserData();
  user_data->id = id;
  user_data->manager = this;

  // initial continue
  send_continue(BUFFER_COUNT, 0);

  std::lock_guard gaurd(parent->watched_sockets_lock);
  parent->watched_sockets[id] = this;
}
void WebSocketManager::force_close() {
#ifdef DEBUG
  printf("force closing\n");
#endif
  {
    std::unique_lock<std::shared_mutex> stream_gaurd(stream_lock);
    std::vector<uint32_t> ids;
    for (auto stream : streams) {
      ids.push_back(stream.first);
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
  send_close(0x02, stream_id);

  if (remove_poll)
    parent->epoll.erase_fd(streams[stream_id].fd);

  // parent->wake();
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

  write(streams.find(packet.stream_id)->second.fd, (void *)packet.data.get(),
        packet.data_len);

  if (streams[packet.stream_id].stream_type != 0x02) {
    streams[packet.stream_id].buffer--;

    // reset message buffer
    if (streams[packet.stream_id].buffer == 0) {
      streams[packet.stream_id].buffer = BUFFER_COUNT;
      send_continue(BUFFER_COUNT, packet.stream_id);
    }
  }

  return streams[packet.stream_id].buffer;
}
void SystemWatcher::watch() {
  while (true) {
    sched_yield();
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
        ssize_t count;
        bool to_close = false;
        std::vector<char> *combined_buffer = new std::vector<char>{};
        do {
          char buf[256];

          int flags = fcntl(events[i].data.fd, F_GETFL, 0);
          count = read(events[i].data.fd, buf, 256);

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

#ifdef DEBUG
          printf("segment %zd\n", count);
#endif

          std::vector<char> holding_buffer(buf, buf + count);
          combined_buffer->insert(combined_buffer->end(),
                                  holding_buffer.begin(), holding_buffer.end());
        } while (true);

        // this mess finds the socket and string to send it
        std::lock_guard sockets_lock(watched_sockets_lock);
        for (auto socket_manager : watched_sockets) {
          std::shared_lock<std::shared_mutex> stream_lock(
              socket_manager.second->stream_lock);

          for (auto stream : socket_manager.second->streams) {
            if (stream.second.fd == events[i].data.fd) {

              uint32_t stream_id = stream.first;
              size_t socket_id = socket_manager.first;

              if (to_close) {
                epoll.erase_fd(events[i].data.fd);
              }

#ifdef DEBUG
              printf("dispatched %zu\n", combined_buffer->size());
#endif

              loop->defer([stream_id, socket_id, this, combined_buffer,
                           to_close]() {
#ifdef DEBUG
                printf("sent data %zu\n", combined_buffer->size());
#endif

                std::lock_guard sockets_lock(watched_sockets_lock);

                if (watched_sockets.find(socket_id) != watched_sockets.end()) {
                  std::shared_lock<std::shared_mutex> stream_lock(
                      watched_sockets[socket_id]->stream_lock);

                  watched_sockets[socket_id]->send_data(
                      stream_id, combined_buffer->data(),
                      combined_buffer->size());
                  delete combined_buffer;

                  if (to_close) {
                    watched_sockets[socket_id]->close_stream(stream_id, false);
                  }
                }
              });
            }
          }
        }
      }

      // main shit
    }
  }
}
