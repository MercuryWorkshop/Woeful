#pragma once
#include "packets.h"
#include "systemInterface.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <sched.h>
#include <shared_mutex>
#include <string_view>
#include <thread>
#include <uWebSockets/App.h>
#include <unistd.h>
#include <unordered_map>

class WebSocketManager;
class SystemWatcher;

struct PerSocketData {
  size_t id;
  WebSocketManager *manager;
};
typedef uWS::WebSocket<false, true, PerSocketData> WebSocket;

struct SocketStreamData {
  uint8_t stream_type;
  uint32_t id;
  int fd;
  std::shared_ptr<addrinfo> fd_info;
  uint32_t buffer = BUFFER_COUNT;
};

class WebSocketManager {
public:
  WebSocketManager(WebSocket *ws, size_t id, SystemWatcher *watcher,
                   SystemInterface *interface);
  ~WebSocketManager();

  void receive(std::string_view view) {
    WispPacket packet((unsigned char *)view.data(), view.size());
    // TODO: log equivelent printf("Got packet type %i\n", packet.packet_type);
    if (packet.packet_type == PACKET_INFO) {
      printf("got info\n");
      if (!info_valid) {
        send_close(0x04, 0);
        ws->close();
        return;
      }
      // TODO: proccess info packet info

      // initial info
      auto info_serialized = InfoPacket(2, 0, {}, 0).serialize();
      auto serialized = WispPacket(PACKET_INFO, 0, info_serialized.first.get(),
                                   info_serialized.second)
                            .serialize();
      ws->send(
          std::string_view((char *)serialized.first.get(), serialized.second));

      received_info = true;
      info_valid = false;
    } else {
      if (!received_info && info_valid) {
        info_valid = false;
      }
      switch (packet.packet_type) {

      case PACKET_CONNECT: {
        auto ret = handle_connect(packet);
        if (!ret.has_value()) {
          // failed to create stream error
          send_close(0x41, packet.stream_id);
        }
        break;
      }
      case PACKET_DATA: {
        printf("got data\n");
        if (!handle_data(packet).has_value()) {
          // TODO: handle error
        }
        break;
      }

      case PACKET_CLOSE: {
        // TODO: report error
        // do nothing, client should close the stream which should close manager
        break;
      }

      default:
        break;
      }
    }
  }

  std::optional<int> handle_connect(WispPacket packet);
  std::optional<uint32_t> handle_data(WispPacket packet);

  void send_close(uint8_t reason, uint32_t stream_id) {
    auto close_serialized = ClosePacket(reason).serialize();
    auto serialized =
        WispPacket(PACKET_CLOSE, stream_id, close_serialized.first.get(),
                   close_serialized.second)
            .serialize();
    ws->send(
        std::string_view((char *)serialized.first.get(), serialized.second));
  }
  void send_data(uint32_t stream_id, char *data, size_t len) {
    auto serialized =
        WispPacket(PACKET_DATA, stream_id, (unsigned char *)data, len)
            .serialize();
    ws->send(
        std::string_view((char *)serialized.first.get(), serialized.second));
  }

  void send_continue(uint32_t buffer, uint32_t stream_id) {
    auto continue_serialized = ContinuePacket(buffer).serialize();
    auto serialized =
        WispPacket(PACKET_CONTINUE, stream_id, continue_serialized.first.get(),
                   continue_serialized.second)
            .serialize();

    ws->send(
        std::string_view((char *)serialized.first.get(), serialized.second));
  }

  void force_close();

  std::shared_mutex stream_lock;
  std::unordered_map<uint32_t, SocketStreamData> streams;
  void close_stream(uint32_t stream_id, bool remove_poll);

private:
  SystemWatcher *parent;
  SystemInterface *interface;
  WebSocket *ws;

  size_t socket_id;

  // wisp info
  bool received_info = false;
  // eg: dont send info packets mid stream
  bool info_valid = true;
};

class SystemWatcher {
public:
  std::mutex watched_sockets_lock;
  std::unordered_map<size_t, WebSocketManager *> watched_sockets;

  SystemWatcher() : loop(uWS::Loop::get()) {
    watcher = std::thread(SystemWatcher::start_thread, this);
  }
  ~SystemWatcher() {
    // clean watcher

    printf("deconstructing\n");
    deconstructing = true;
    wake();
    watcher.join();
  }

  void watch();
  static void start_thread(SystemWatcher *watcher) { watcher->watch(); }

  // can be called by main thread
  void wake() { epoll.wake(); }

  EpollWrapper epoll;

private:
  // for killing the other thread
  std::thread watcher;
  std::atomic<bool> deconstructing = false;
  uWS::Loop *loop;
};
