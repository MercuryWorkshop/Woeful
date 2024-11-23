#pragma once
#include "packets.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <sched.h>
#include <string_view>
#include <thread>
#include <uWebSockets/App.h>
#include <unordered_map>

class WebSocketManager;
class SystemWatcher;

struct PerSocketData {
  size_t id;
  WebSocketManager *manager;
};
typedef uWS::WebSocket<false, true, PerSocketData> WebSocket;

struct SocketStream {
  FILE fd;
  uint32_t buffer = BUFFER_COUNT;
  uint32_t id;
};

class WebSocketManager {
public:
  WebSocketManager(WebSocket *ws, size_t id, SystemWatcher *watcher);
  ~WebSocketManager();

  void receive(std::string_view view) {
    WispPacket packet((unsigned char *)view.data(), view.size());
    printf("Got packet type %i\n", packet.packet_type);
    if (packet.packet_type == PACKET_INFO) {
      // TODO: proccess info packet info
      received_info = true;
    } else {
      if (!received_info) {
        // TODO: report error;
        send_close(0x04);
        force_close();
        return;
      }
      switch (packet.packet_type) {

      case PACKET_CONNECT: {
        break;
      }
      case PACKET_DATA: {
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

  void handle_connect(WispPacket packet);
  void handle_data(WispPacket packet);

  void send_close(uint8_t reason) {
    auto close_serialized = ClosePacket(reason).serialize();
    auto serialized = WispPacket(PACKET_CLOSE, 0, close_serialized.first.get(),
                                 close_serialized.second)
                          .serialize();
    ws->send(
        std::string_view((char *)serialized.first.get(), serialized.second));
  }

  void force_close() {
    ws->close();
    delete this;
  }

private:
  SystemWatcher *parent;
  WebSocket *ws;

  // wisp info
  bool received_info = false;
  std::unordered_map<uint32_t, SocketStream> streams;
};

class SystemWatcher {
public:
  std::unordered_map<size_t, WebSocketManager *> watched_sockets;
  SystemWatcher() { watcher = std::thread(SystemWatcher::start_thread, this); }
  ~SystemWatcher() {
    // clean watcher
    quit_watcher.lock();
    deconstructing = true;
    quit_watcher.unlock();
    watcher.join();
  }

  void watch() {
    while (true) {
      // make killable
      std::lock_guard guard(quit_watcher);
      if (deconstructing) {
        break;
      }

      // main loop
      sched_yield();
    }
  }
  static void start_thread(SystemWatcher *watcher) { watcher->watch(); }

private:
  // for killing the other thread
  std::mutex quit_watcher;
  std::thread watcher;
  bool deconstructing = false;
};
