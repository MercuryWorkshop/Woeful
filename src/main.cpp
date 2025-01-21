#include "CLI/App.hpp"
#include "CLI/CLI.hpp"
#include "server.h"
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <thread>

int main(int argc, char *argv[]) {

  CLI::App app;

  int num_port = 9001;
  app.add_option("--port", num_port, "The port the server binds to.")
      ->default_val(9001)
      ->envname("PORT");

  std::string filename;
  app.add_option("--config", filename,
                 "Config file, overwrites other cli args.");

  std::string descr = "Woeful is a C++ wisp server that might not suck.";
#ifdef PCAP
  descr += "\nPCAP generation enabled!";
#endif
  app.description(descr);

  CLI11_PARSE(app, argc, argv);

  if (std::thread::hardware_concurrency() < 3) {
    fprintf(stderr, "%s: Program requires at least 3 threads\n", argv[0]);
    exit(-1);
  }

  if (filename != "") {
    auto conf = read_config((char *)filename.c_str());
    if (!conf.has_value()) {
      fprintf(stderr, "%s: %s\n", argv[0], conf.error().c_str());
      exit(-1);
    }
    set_conf(conf.value());
  } else {
    set_conf({.pcap_capture = false, .port_number = num_port});
  }

  // TODO: cli args
  start_wisp_server();
}
