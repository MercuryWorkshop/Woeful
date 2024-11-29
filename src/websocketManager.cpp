
#include "websocketManager.hpp"
#include "packets.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <queue>
#include <sched.h>
#include <shared_mutex>
#include <string_view>
#include <sys/epoll.h>
#include <tuple>
#include <unistd.h>
#include <vector>

WebSocketManager::WebSocketManager(WebSocket *ws, size_t id,
                                   SystemWatcher *watcher,
                                   SystemInterface *interface)
    : ws(ws), parent(watcher), interface(interface) {
  auto user_data = ws->getUserData();
  user_data->id = id;
  user_data->manager = this;

  // initial info
  auto info_serialized = InfoPacket(2, 0, {}, 0).serialize();
  auto serialized = WispPacket(PACKET_INFO, 0, info_serialized.first.get(),
                               info_serialized.second)
                        .serialize();
  ws->send(std::string_view((char *)serialized.first.get(), serialized.second));
  // initial continue
  send_continue(BUFFER_COUNT, 0);

  std::lock_guard gaurd(parent->watched_sockets_lock);
  parent->watched_sockets[id] = this;
}
void WebSocketManager::force_close() {
  printf("force closing\n");
  {
    std::unique_lock<std::shared_mutex> stream_gaurd(stream_lock);
    for (auto stream : streams) {
      close_stream(stream.first);
    }

    std::lock_guard gaurd(parent->watched_sockets_lock);
    parent->watched_sockets.erase(ws->getUserData()->id);
  }
  delete this;
}

WebSocketManager::~WebSocketManager() {}
void WebSocketManager::close_stream(uint32_t stream_id) {
  // std::unique_lock<std::shared_mutex> gaurd(stream_lock);
  parent->epoll.erase_fd(streams[stream_id].fd);
  // parent->wake();
  close(streams[stream_id].fd);
  // streams.erase(stream_id);
}

std::optional<int> WebSocketManager::handle_connect(WispPacket packet) {
  uint32_t stream_id = packet.stream_id;
  ConnectPacket connect_packet(packet.data.get(), packet.data_len);
  uint8_t stream_type = connect_packet.stream_type;
  printf("got connect %s\n", connect_packet.destination_hostname.get());

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
        printf("woken\n");
        char buf[10];
        read(events[i].data.fd, buf, 10);
        continue;
      } else {
        ssize_t count;
        std::vector<char> combined_buffer;
        do {
          char buf[256];
          count = read(events[i].data.fd, buf, 256);

          if (count <= 0)
            break;

          std::vector<char> holding_buffer(buf, buf + count);
          combined_buffer.insert(combined_buffer.end(), holding_buffer.begin(),
                                 holding_buffer.end());
        } while (count > 0);

        printf("message is %zu\n", combined_buffer.size());
      }

      // main shit
    }
  }
}
