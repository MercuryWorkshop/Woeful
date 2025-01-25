#include "packets.h"
#include "server.h"
#include <arpa/inet.h>
#include <bits/types/res_state.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <optional>
#include <resolv.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

class SystemInterface {
public:
  SystemInterface(char *addr_ip) {
    // TODO: set custom dns server
  }

  ~SystemInterface() {}

  std::optional<std::shared_ptr<addrinfo>> resolve(char *hostname, char *port,
                                                   int sock_type) {
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = sock_type;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    auto retval = getaddrinfo(hostname, port, &hints, &result);

    if (retval != 0) {
      return {};
    }

    std::shared_ptr<addrinfo> ret(result, freeaddrinfo);
    return ret;
  }

  std::optional<std::pair<int, std::shared_ptr<addrinfo>>>
  open_stream(char *hostname, uint8_t type, uint16_t port) {
    int sock_type = -1;
    switch (type) {
    case 0x01:
      sock_type = SOCK_STREAM;
      break;
    case 0x02:
      sock_type = SOCK_DGRAM;
      break;
    default:
      return {};
    }

    bool is_matched_hostname = false;
    bool is_matched_port = false;
    auto conf = get_conf();
    for (auto member : conf->block_members) {
      switch (member.type) {
      case WoefulConfig::Member::MEMBER_HOSTNAME: {
        if (member.data.hostname == hostname) {
          is_matched_hostname = true;
        }
        break;
      }

      case WoefulConfig::Member::MEMBER_PORT: {
        if (member.data.port == port) {
          is_matched_port = true;
        }
        break;
      }
      }
    }

    if (conf->block_type == conf->BLOCK_BLACK &&
        (is_matched_hostname || is_matched_port))
      return {};
    if (conf->block_type == conf->BLOCK_WHITE &&
        !(is_matched_hostname && is_matched_port))
      return {};

    std::string port_str = std::to_string(port);
    auto dns = resolve(hostname, (char *)port_str.c_str(), sock_type);
    if (!dns.has_value()) {
      return {};
    }

    int fd = socket(AF_INET, sock_type, 0);
    if (fd == -1) {
      return {};
    }

    auto ret = connect(fd, dns->get()->ai_addr, dns->get()->ai_addrlen);
    if (ret != 0) {
      int errsv = errno;
#ifdef DEBUG
      printf("failed to connect %d\n", errsv);
#endif
      close(fd);
      return {};
    }

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    return (std::pair<int, std::shared_ptr<addrinfo>>){fd, dns.value()};
  }

private:
};

struct EpollWrapper {
  int epfd;
  int pipes[2];
  EpollWrapper() : epfd(epoll_create1(0)) {
    pipe(pipes);
    push_fd(pipes[0]);
  }
  ~EpollWrapper() {
    close(epfd);
    close(pipes[0]);
    close(pipes[1]);
  }
  std::optional<int> push_fd(int fd) {
    epoll_event event = {.events = EPOLLIN,
                         .data = {
                             .fd = fd,
                         }};
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    return fd;
  }
  std::optional<int> erase_fd(int fd) {
    epoll_event event = {.events = EPOLLIN,
                         .data = {
                             .fd = fd,
                         }};
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event) != 0)
      return {};
    return fd;
  }

  // create pipe event
  void wake() { write(pipes[1], "w", 2); }

  // must request events with length 127
  std::optional<int> wait(epoll_event *events) {
    int event_count = epoll_wait(epfd, events, BUFFER_COUNT, -1);
    return event_count;
  }
};
