#include "CLI/App.hpp"
#include "CLI/CLI.hpp"
#include "server.h"
#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char *argv[]) {

  CLI::App app;

  int num_port = 9001;
  app.add_option("--port", num_port, "The port the server binds to.")
      ->default_val(9001)
      ->envname("PORT");

  app.description("Woeful is a C++ wisp server that might not suck.");

  CLI11_PARSE(app, argc, argv);

  // TODO: cli args
  start_wisp_server(num_port);
}
