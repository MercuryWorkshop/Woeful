#include "../src/packets.h"
#include "../src/server.h"
#include "../src/systemInterface.h"
#include "gtest/gtest.h"
#include <arpa/inet.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

TEST(WispPacket, Parsing) {
  unsigned char data[] = {9, 8, 0, 0, 0, 1, 2, 3};
  WispPacket packet(data, 8);
  EXPECT_EQ(packet.packet_type, 9);
  EXPECT_EQ(packet.stream_id, 8);
  EXPECT_EQ(packet.data[0], 1);
  EXPECT_EQ(packet.data[1], 2);
  EXPECT_EQ(packet.data[2], 3);
  EXPECT_EQ(packet.data_len, 3);
}
TEST(WispPacket, Generating) {
  unsigned char data[] = {1, 2, 3};
  WispPacket packet(9, 8, data, 3);

  EXPECT_EQ(packet.packet_type, 9);
  EXPECT_EQ(packet.stream_id, 8);
  EXPECT_EQ(packet.data[0], 1);
  EXPECT_EQ(packet.data[1], 2);
  EXPECT_EQ(packet.data[2], 3);
  EXPECT_EQ(packet.data_len, 3);
}
TEST(WispPacket, Serializing) {
  unsigned char data[] = {9, 8, 0, 0, 0, 1, 2, 3};
  WispPacket packet(data, 8);
  auto serialized = packet.serialize();

  EXPECT_EQ(8, serialized.second);
  for (size_t i = 0; i < 8; i++) {
    EXPECT_EQ(data[i], serialized.first.get()[i]);
  }
}
TEST(ConnectPacket, Parsing) {
  unsigned char data[] = {1,   80,  0,   'f', 'o', 'x', 'm', 'o',
                          's', 's', '.', 'c', 'o', 'm', 0};
  ConnectPacket packet(data, 15);
  EXPECT_EQ(packet.stream_type, 1);
  EXPECT_EQ(packet.destination_port, 80);
  EXPECT_EQ(strcmp((char *)packet.destination_hostname.get(), "foxmoss.com"),
            0);
}
TEST(ConnectPacket, RealWorld) {
  unsigned char data[] = {0x01, 0x02, 0x00, 0x00, 0x00, 0x01, 0xbb,
                          0x01, 0x66, 0x6f, 0x6e, 0x74, 0x73, 0x2e,
                          0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x61,
                          0x70, 0x69, 0x73, 0x2e, 0x63, 0x6f, 0x6d};
  WispPacket packet(data, 28);
  EXPECT_EQ(packet.packet_type, PACKET_CONNECT);
  ConnectPacket connect_packet(packet.data.get(), packet.data_len);
  EXPECT_EQ(strcmp((char *)connect_packet.destination_hostname.get(),
                   "fonts.googleapis.com"),
            0);
}

TEST(DataPacket, Parsing) {
  unsigned char data[] = "Hello world!";
  DataPacket packet(data, strlen((char *)data) + 1);
  EXPECT_EQ(strcmp((char *)packet.stream_payload.get(), "Hello world!"), 0);
}
TEST(ContinuePacket, Parsing) {
  unsigned char data[] = {0x15, 0xCD, 0x5B, 0x07};
  ContinuePacket packet(data, 4);
  EXPECT_EQ(packet.buffer_remaining, 123456789);
}
TEST(ContinuePacket, Generating) {
  ContinuePacket packet(123456789);
  EXPECT_EQ(packet.buffer_remaining, 123456789);
}
TEST(ClosePacket, Parsing) {
  unsigned char data[] = {0x41};
  ClosePacket packet(data, 1);
  EXPECT_EQ(packet.close_reason, 0x41);
}
TEST(ClosePacket, Generating) {
  ClosePacket packet(0x41);
  EXPECT_EQ(packet.close_reason, 0x41);
}

TEST(InfoPacket, Parsing) {
  unsigned char data[] = {9, 17};
  InfoPacket packet(data, 2);
  EXPECT_EQ(packet.major_wisp_version, 9);
  EXPECT_EQ(packet.minor_wisp_version, 17);
  EXPECT_EQ(packet.extension_data_len, 0);
}
TEST(InfoPacket, Generating) {
  unsigned char data[] = {1, 2, 3};
  InfoPacket packet(9, 17, data, 3);
  EXPECT_EQ(packet.major_wisp_version, 9);
  EXPECT_EQ(packet.minor_wisp_version, 17);
  EXPECT_EQ(packet.extension_data_len, 3);
}
TEST(InfoPacket, Serializing) {
  unsigned char data[] = {9, 17};
  InfoPacket packet(data, 2);
  auto serialized = packet.serialize();
  EXPECT_EQ(serialized.second, 2);
  EXPECT_EQ(serialized.first.get()[0], 9);
  EXPECT_EQ(serialized.first.get()[1], 17);
}

TEST(SystemInterface, Resolution) {
  SystemInterface interface((char *)"1.1.1.1");
  {
    auto res =
        interface.resolve((char *)"foxmoss.com", (char *)"80", SOCK_STREAM);
    EXPECT_EQ(
        strcmp(inet_ntoa(((struct sockaddr_in *)res->get()->ai_addr)->sin_addr),
               "205.185.125.167"),
        0);
  }
  {
    auto res =
        interface.resolve((char *)"45.79.112.203", (char *)"4242", SOCK_STREAM);
    EXPECT_EQ(
        strcmp(inet_ntoa(((struct sockaddr_in *)res->get()->ai_addr)->sin_addr),
               "45.79.112.203"),
        0);
  }
  // TODO: test for ipv6?? i genuinely cant find ipv6 only site
}

TEST(SystemInterface, Epoll) {
  // EpollWrapper epoll;
  // std::atomic<bool> should_kill = false;
  // std::atomic<int> wake_count = 0;
  // auto thread = std::thread([&epoll, &should_kill, &wake_count]() {
  //   while (true) {
  //     epoll_event events[BUFFER_COUNT];
  //     epoll.wait(events);
  //     wake_count++;
  //     if (should_kill) {
  //       return;
  //     }
  //   }
  // });
  // should_kill = true;
  // epoll.wake();
  // thread.join();
  // EXPECT_EQ(wake_count, 1);
}

TEST(SystemInterface, ReadNonBlock) {
  // EpollWrapper epoll;
  // SystemInterface interface((char *)"1.1.1.1");
  // auto stream = interface.open_stream((char *)"127.0.0.1", 1, 8080);
  // EXPECT_EQ(stream.has_value(), true);
  //
  // epoll.push_fd(stream->first);
  // char *buf = (char *)"Hello!\n";
  // write(stream->first, buf, strlen(buf));
  // epoll_event events[BUFFER_COUNT];
  // auto count = epoll.wait(events);
  // EXPECT_EQ(count.has_value(), true);
  // EXPECT_EQ(count, 1);
  //
  // char read_buf[1024];
  // read(events[0].data.fd, read_buf, 1024);
  // printf("%s\n", read_buf);
  //
  // close(stream->first);
}

TEST(Config, Basic) {
  auto conf = read_config((char *)"../tests/basic.xml");
  EXPECT_EQ(conf.has_value(), true);
  EXPECT_EQ(conf.value().pcap_capture, true);
  EXPECT_EQ(conf.value().port_number, 8080);
}

TEST(Config, BadValidation) {
  auto conf = read_config((char *)"../tests/bad_validation.xml");
  EXPECT_EQ(conf.has_value(), false);
}
TEST(Config, BadElement) {
  auto conf = read_config((char *)"../tests/bad_element.xml");
  EXPECT_EQ(conf.has_value(), false);
}
TEST(Config, Blacklist) {
  auto conf = read_config((char *)"../tests/blacklist.xml");
  EXPECT_EQ(conf.has_value(), true);
  EXPECT_EQ(conf.value().block_type, WoefulConfig::BLOCK_BLACK);
  EXPECT_EQ(conf.value().block_members.size(), 2);
  EXPECT_EQ(conf.value().block_members[0].type,
            WoefulConfig::Member::MEMBER_HOSTNAME);
  EXPECT_EQ(conf.value().block_members[0].data.hostname, "example.com");
  EXPECT_EQ(conf.value().block_members[1].type,
            WoefulConfig::Member::MEMBER_PORT);
  EXPECT_EQ(conf.value().block_members[1].data.port, 22);
}
