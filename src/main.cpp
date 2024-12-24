#include "CLI/App.hpp"
#include "CLI/CLI.hpp"
#include "server.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

int main(int argc, char *argv[]) {

  CLI::App app;

  int num_port = 9001;
  app.add_option("--port", num_port, "The port the server binds to.")
      ->default_val(9001)
      ->envname("PORT");

  std::string descr = "Woeful is a C++ wisp server that might not suck.";
#ifdef PCAP
  descr += "\nPCAP generation enabled!!";
#endif
  app.description(descr);

  CLI11_PARSE(app, argc, argv);

  if (std::thread::hardware_concurrency() < 3) {
    fprintf(stderr, "%s: Program requires at least 3 threads\n", argv[0]);
    exit(-1);
  }

  // TODO: cli args
  start_wisp_server(num_port);
}
