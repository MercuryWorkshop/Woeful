#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <sys/types.h>
#include <tinyxml2.h>
#include <tl/expected.hpp>

void start_wisp_server();

struct WoefulConfig {
  bool pcap_capture = false;
  std::optional<unsigned int> port_number;
};

WoefulConfig *get_conf();
void set_conf(WoefulConfig conf);

tl::expected<WoefulConfig, std::string> read_config(char *filename);
