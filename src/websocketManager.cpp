
#include "websocketManager.hpp"
#include "packets.h"
#include <cstdio>

WebSocketManager::WebSocketManager(WebSocket *ws, size_t id,
                                   SystemWatcher *watcher)
    : ws(ws), parent(watcher) {
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
  auto continue_serialized = ContinuePacket(BUFFER_COUNT).serialize();
  serialized = WispPacket(PACKET_CONTINUE, 0, continue_serialized.first.get(),
                          continue_serialized.second)
                   .serialize();

  ws->send(std::string_view((char *)serialized.first.get(), serialized.second));

  parent->watched_sockets[id] = this;
}
WebSocketManager::~WebSocketManager() {
  parent->watched_sockets.erase(
      parent->watched_sockets.find(ws->getUserData()->id));
}
