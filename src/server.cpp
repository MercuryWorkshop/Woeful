#include "websocketManager.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <libusockets.h>
#include <signal.h>
#include <string_view>
#include <sys/socket.h>
#include <uWebSockets/App.h>
#include <uWebSockets/Loop.h>
#include <uWebSockets/WebSocketProtocol.h>
#include <uv.h>

size_t id = 0;
SystemInterface system_interface((char *)"1.1.1.1");
SystemWatcher system_watcher;

bool interupted = false;
// what does the int in this func do?
void int_handler(int) { interupted = true; }

void start_wisp_server(int port) {
  signal(SIGINT, int_handler);

  auto app = uWS::App().ws<PerSocketData>(
      "*", {
               .compression = uWS::CompressOptions(
                   uWS::DEDICATED_COMPRESSOR_4KB | uWS::DEDICATED_DECOMPRESSOR),
               .maxPayloadLength = 100 * 1024 * 1024,
               .idleTimeout = 16,
               .maxBackpressure = MAX_BACKPRESSURE,
               .closeOnBackpressureLimit = false,
               .resetIdleTimeoutOnSend = false,
               .sendPingsAutomatically = true,

               .upgrade = nullptr,
               .open =
                   [](uWS::WebSocket<false, true, PerSocketData> *ws) {
                     new WebSocketManager(ws, id++, &system_watcher,
                                          &system_interface);
                   },

               .message =
                   [](uWS::WebSocket<false, true, PerSocketData> *ws,
                      std::string_view message, uWS::OpCode op_code) {
#ifdef DEBUG
                     if (op_code != uWS::OpCode::BINARY) {
                       // TODO: handle error
                       printf("websocket packet was not binary\n");
                     }
#endif
                     ws->getUserData()->manager->receive(message);
                   },
               .drain =
                   [](uWS::WebSocket<false, true, PerSocketData> *ws) {
#ifdef DEBUG
                     printf("drained\n");
#endif
                     ws->getUserData()->manager->has_backpressure = false;
                     ws->getUserData()->manager->has_backpressure.notify_all();
                     ws->getUserData()->manager->update_streams();

		     system_watcher.wake();
                   },
               .close =
                   [](uWS::WebSocket<false, true, PerSocketData> *ws, int val,
                      std::string_view error) {
#ifdef DEBUG
                     printf("got closed\n");
#endif
                     ws->getUserData()->manager->force_close();
                     delete ws->getUserData()->manager;
                   },

           });
  app.listen(port,
             [&app, port](us_listen_socket_t *listen_socket) {
               uWS::Loop::get()->addPostHandler(
                   NULL, [listen_socket, &app](uWS::Loop *) {
                     if (interupted) {
#ifdef DEBUG
                       printf("killer app\n");
#endif
                       app.close();
                       interupted = false; // dont DUP! closing
                       return;
                     }
                     // TODO: create equivelent log
                     // printf("Sockets curently open:\n");
                     // for (auto websocket : system_watcher.watched_sockets) {
                     //   printf("Socket %lu\n", websocket.first);
                     // }
                     // printf("\n");
                   });

               if (listen_socket) {
                 printf("Listening to %i\n", port);
               } else {
                 printf("Failed to bind\n");
               }
             })
      .run();
}
