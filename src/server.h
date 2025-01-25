#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <sys/types.h>
#include <tinyxml2.h>
#include <tl/expected.hpp>
#include <vector>

void start_wisp_server();

struct WoefulConfig {
  bool pcap_capture = false;
  std::optional<unsigned int> port_number;

  enum BlockType {
    BLOCK_NONE,
    BLOCK_WHITE,
    BLOCK_BLACK
  } block_type = BLOCK_NONE;

  struct Member {
    enum MemberType { MEMBER_HOSTNAME, MEMBER_PORT } type;
    union MemberUnion {
      std::string hostname;
      unsigned int port;

      MemberUnion() {}
      ~MemberUnion() {}
    } data;

    Member(std::string str) : type(MEMBER_HOSTNAME), data() {
      new (&data.hostname) std::string(str);
    }
    Member(unsigned int port) : type(MEMBER_PORT), data() { data.port = port; }

    Member(const Member &b) : type(b.type) {
      switch (b.type) {
      case MEMBER_HOSTNAME:
        new (&data.hostname) std::string(b.data.hostname);
        break;
      case MEMBER_PORT:
        data.port = b.data.port;
        break;
      }
    }

    Member &operator=(const Member &b) {
      if (this != &b) {
        if (type == MEMBER_HOSTNAME) {
          data.hostname.~basic_string();
        }

        type = b.type;
        switch (b.type) {
        case MEMBER_HOSTNAME:
          new (&data.hostname) std::string(b.data.hostname);
          break;
        case MEMBER_PORT:
          data.port = b.data.port;
          break;
        }
      }
      return *this;
    }

    ~Member() {
      if (type == MEMBER_HOSTNAME) {
        data.hostname.~basic_string();
      }
    }
  };

  std::vector<Member> block_members;
};

WoefulConfig *get_conf();
void set_conf(WoefulConfig conf);

tl::expected<WoefulConfig, std::string> read_config(char *filename);
