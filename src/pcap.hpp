#pragma once
#include <EthLayer.h>
#include <IPv4Layer.h>
#include <MacAddress.h>
#include <Packet.h>
#include <PayloadLayer.h>
#include <PcapDevice.h>
#include <PcapFileDevice.h>
#include <SystemUtils.h>
#include <TcpLayer.h>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

class PcapInterface {
private:
  pcpp::PcapNgFileWriterDevice out;
  std::mutex writer_gaurd;
  std::string user_ip = "192.168.0.1";
  std::string target_ip = "10.0.0.1";
  std::string user_mac = "00:50:43:11:22:33";
  std::string target_mac = "aa:bb:cc:dd:ee:ff";
  uint16_t user_port = 1000;
  uint16_t target_port = 2000;

public:
  PcapInterface(std::string file) : out(file) {
    out.open();
    printf("opening pcap %s\n", out.getFileName().c_str());
  }
  ~PcapInterface() {
    out.close();
    printf("wrote pcap to %s\n", out.getFileName().c_str());
  }
  void write_tcp(std::string src_mac, std::string dest_mac, std::string src,
                 std::string dest, pcpp::TcpLayer tcp_layer,
                 const uint8_t *data, size_t len) {
    pcpp::EthLayer ethernet_layer((pcpp::MacAddress(src_mac)),
                                  pcpp::MacAddress(dest_mac));
    pcpp::IPv4Layer ipv4_layer((pcpp::IPv4Address(src)),
                               pcpp::IPv4Address(dest));
    ipv4_layer.getIPv4Header()->ipId = pcpp::hostToNet16(2000);
    ipv4_layer.getIPv4Header()->timeToLive = 64;
    pcpp::PayloadLayer payload_layer(data, len);

    pcpp::Packet packet(100);
    packet.addLayer(&ethernet_layer);
    packet.addLayer(&ipv4_layer);
    packet.addLayer(&tcp_layer);
    packet.addLayer(&payload_layer);
    packet.computeCalculateFields();
    std::lock_guard lock(writer_gaurd);
    out.writePacket(*packet.getRawPacket());
  }
  // data from user
  pcpp::TcpLayer user_tcp_layer = pcpp::TcpLayer(user_port, target_port);
  void write_dummy_user(const uint8_t *data, size_t data_len) {
    write_tcp(user_mac, target_mac, user_ip, target_ip, user_tcp_layer, data,
              data_len);
  }
  // data from target
  pcpp::TcpLayer target_tcp_layer = pcpp::TcpLayer(target_port, user_port);
  void write_dummy_target(const uint8_t *data, size_t data_len) {
    write_tcp(target_mac, user_mac, target_ip, user_ip, user_tcp_layer, data,
              data_len);
  }
};
