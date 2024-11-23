#include "../src/packets.h"
#include <cstring>
#include <gtest/gtest.h>

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
