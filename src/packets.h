#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

#define BUFFER_COUNT 127
#define SEGMENT_SIZE 1024 * 20
#define MAX_WS_FRAME 63 * 1000000
#define MAX_POSSIBLE_WS_FRAME 64 * 1000000
#define MAX_QUEUE 1000

enum PacketType {
  PACKET_CONNECT = 0x01,
  PACKET_DATA = 0x02,
  PACKET_CONTINUE = 0x03,
  PACKET_CLOSE = 0x04,
  PACKET_INFO = 0x05,
};

struct WispPacket {
  uint8_t packet_type;
  uint32_t stream_id;
  std::shared_ptr<unsigned char[]> data;
  size_t data_len;

  WispPacket(unsigned char *src, size_t src_length) {
    packet_type = *(uint8_t *)(src);
    stream_id = *(uint32_t *)(src + sizeof(uint8_t));

    size_t header_size = sizeof(uint8_t) + sizeof(uint32_t);
    unsigned char *data_ptr = src + header_size;

    data_len = src_length - header_size;

    data = std::make_unique_for_overwrite<unsigned char[]>(data_len);
    memcpy(data.get(), data_ptr, data_len);
  }
  WispPacket(uint8_t packet_type, uint32_t stream_id, unsigned char *data_src,
             size_t data_len)
      : packet_type(packet_type), stream_id(stream_id), data_len(data_len) {
    data = std::make_unique_for_overwrite<unsigned char[]>(data_len);
    memcpy(data.get(), data_src, data_len);
  }
  WispPacket(uint8_t packet_type, uint32_t stream_id,
             std::shared_ptr<unsigned char[]> &data_src, size_t data_len)
      : packet_type(packet_type), stream_id(stream_id),
        data(std::move(data_src)), data_len(data_len) {}
  ~WispPacket() {}

  std::pair<std::shared_ptr<unsigned char[]>, size_t> serialize() {
    size_t build_size = sizeof(uint8_t) + sizeof(uint32_t) + data_len;
    unsigned char *build = new unsigned char[build_size];

    *(uint8_t *)(build) = packet_type;
    *(uint32_t *)(build + sizeof(uint8_t)) = stream_id;
    mempcpy((build + build_size - data_len), data.get(), data_len);

    return std::pair<std::shared_ptr<unsigned char[]>, size_t>(
        std::shared_ptr<unsigned char[]>(build), build_size);
  }
};

struct InfoPacket {
  uint8_t major_wisp_version;
  uint8_t minor_wisp_version;

  std::unique_ptr<unsigned char[]> extension_data;
  size_t extension_data_len;

  InfoPacket(unsigned char *src, size_t src_length) {
    major_wisp_version = *(uint8_t *)(src);
    minor_wisp_version = *(uint8_t *)(src + sizeof(uint8_t));

    size_t header_size = sizeof(uint8_t) + sizeof(uint8_t);
    unsigned char *data_ptr = src + header_size;

    extension_data_len = src_length - header_size;
    extension_data =
        std::make_unique_for_overwrite<unsigned char[]>(extension_data_len);

    memcpy(extension_data.get(), data_ptr, extension_data_len);
  }
  InfoPacket(uint8_t major_wisp_version, uint8_t minor_wisp_version,
             unsigned char *src, size_t src_length)
      : major_wisp_version(major_wisp_version),
        minor_wisp_version(minor_wisp_version) {
    extension_data_len = src_length;
    extension_data =
        std::make_unique_for_overwrite<unsigned char[]>(src_length);

    memcpy(extension_data.get(), src, extension_data_len);
  }

  std::pair<std::shared_ptr<unsigned char[]>, size_t> serialize() {
    size_t build_size = sizeof(uint8_t) + sizeof(uint8_t) + extension_data_len;
    auto build = new unsigned char[build_size];
    *(uint8_t *)(build) = major_wisp_version;
    *(uint8_t *)(build + sizeof(uint8_t)) = minor_wisp_version;
    memcpy((void *)(build + build_size - extension_data_len),
           extension_data.get(), extension_data_len);

    return std::pair<std::shared_ptr<unsigned char[]>, size_t>(
        std::shared_ptr<unsigned char[]>(build), build_size);
  }
};

struct ConnectPacket {
  uint8_t stream_type;
  uint16_t destination_port;

  std::unique_ptr<unsigned char[]> destination_hostname;
  size_t destination_hostname_len;

  ConnectPacket(unsigned char *src, size_t src_length) {
    stream_type = *(uint8_t *)(src);
    destination_port = *(uint16_t *)(src + sizeof(uint8_t));

    size_t header_size = sizeof(uint8_t) + sizeof(uint16_t);
    unsigned char *hostname_ptr = src + header_size;

    destination_hostname_len = src_length - header_size;

    if (src[src_length - 1] != 0) { // hostname must be null terminated
      destination_hostname_len += 1;
    }

    destination_hostname = std::make_unique_for_overwrite<unsigned char[]>(
        destination_hostname_len);

    if (src[src_length - 1] != 0) { // hostname must be null terminated
      destination_hostname.get()[destination_hostname_len - 1] = 0;
    }

    memcpy(destination_hostname.get(), hostname_ptr, src_length - header_size);
  }
  // TODO: create server side generater, although it is only supposed to be
  // generated client side
};

struct DataPacket {
  std::unique_ptr<unsigned char[]> stream_payload;
  size_t stream_payload_len;

  DataPacket(unsigned char *src, size_t src_length) {
    stream_payload_len = src_length;
    stream_payload =
        std::make_unique_for_overwrite<unsigned char[]>(src_length);

    memcpy(stream_payload.get(), src, src_length);
  }
};

struct ContinuePacket {
  uint32_t buffer_remaining;

  ContinuePacket(unsigned char *src, size_t src_length) {
    buffer_remaining = *(uint32_t *)(src);
  }
  ContinuePacket(uint32_t buffer_remaining)
      : buffer_remaining(buffer_remaining) {}

  std::pair<std::shared_ptr<unsigned char[]>, size_t> serialize() {
    size_t build_size = sizeof(uint32_t);
    unsigned char *build = new unsigned char[build_size];

    *(uint32_t *)(build) = buffer_remaining;

    return std::pair<std::shared_ptr<unsigned char[]>, size_t>(
        std::shared_ptr<unsigned char[]>(build), build_size);
  }
};

struct ClosePacket {
  uint8_t close_reason;

  ClosePacket(unsigned char *src, size_t src_length) {
    close_reason = *(uint8_t *)(src);
  }
  ClosePacket(uint8_t close_reason) : close_reason(close_reason) {}
  std::pair<std::shared_ptr<unsigned char[]>, size_t> serialize() {
    size_t build_size = sizeof(uint8_t);
    unsigned char *build = new unsigned char[build_size];

    *(uint8_t *)(build) = close_reason;

    return std::pair<std::shared_ptr<unsigned char[]>, size_t>(
        std::shared_ptr<unsigned char[]>(build), build_size);
  }
};
