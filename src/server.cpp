#include "packets.h"
#include "websocketManager.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <libusockets.h>
#include <queue>
#include <signal.h>
#include <string_view>
#include <sys/socket.h>
#include <system_error>
#include <thread>
#include <uWebSockets/App.h>
#include <uWebSockets/Loop.h>
#include <unordered_map>
#include <uv.h>

size_t id = 0;
SystemWatcher system_watcher;

bool interupted = false;
void int_handler(int dummy) { interupted = true; }

void start_wisp_server() {
  signal(SIGINT, int_handler);

  auto app = uWS::App().ws<PerSocketData>(
      "*",
      {.compression = uWS::CompressOptions(uWS::DEDICATED_COMPRESSOR_4KB |
                                           uWS::DEDICATED_DECOMPRESSOR),
       .maxPayloadLength = 100 * 1024 * 1024,
       .idleTimeout = 16,
       .maxBackpressure = 100 * 1024 * 1024,
       .closeOnBackpressureLimit = false,
       .resetIdleTimeoutOnSend = false,
       .sendPingsAutomatically = true,

       .upgrade = nullptr,
       .open =
           [](uWS::WebSocket<false, true, PerSocketData> *ws) {
             auto manager = new WebSocketManager(ws, id++, &system_watcher);
           },

       .message =
           [](uWS::WebSocket<false, true, PerSocketData> *ws,
              std::string_view message, uWS::OpCode opCode) {
             ws->getUserData()->manager->receive(message);
           },
       .close =
           [](uWS::WebSocket<false, true, PerSocketData> *ws, int val,
              std::string_view error) { delete ws->getUserData()->manager; }

      });
  app.listen(9001,
             [&app](us_listen_socket_t *listen_socket) {
               uWS::Loop::get()->addPostHandler(
                   NULL, [listen_socket, &app](uWS::Loop *) {
                     if (interupted) {
                       app.close();
                       return;
                     }
                     printf("Sockets curently open:\n");
                     for (auto websocket : system_watcher.watched_sockets) {
                       printf("Socket %lu\n", websocket.first);
                     }
                     printf("\n");
                   });

               if (listen_socket) {
                 std::cout << "Listening on port " << 9001 << std::endl;
               } else {
                 std::cout << "Failed to load certs or to bind to port"
                           << std::endl;
               }
             })
      .run();
}
