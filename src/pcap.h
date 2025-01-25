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
#include <limits>
#include <mutex>
#include <netinet/in.h>
#include <string>

class PcapInterface {
private:
  pcpp::PcapNgFileWriterDevice out;
  std::mutex writer_gaurd;
  const std::string user_ip = "192.168.0.1";
  const std::string target_ip = "10.0.0.1";

  // random, iirc these dont actually matter
  const std::string user_mac = "4A:7D:3D:39:61:79";
  const std::string target_mac = "16:F7:AD:6C:C1:57";

  const uint16_t user_port = 1000;
  const uint16_t target_port = 2000;

  uint32_t user_sequence_number = 3;
  uint32_t target_sequence_number = 3;

public:
  PcapInterface(std::string file, std::string target_ip, uint16_t target_port)
      : out(file), target_ip(target_ip), target_port(target_port) {
    out.open();
    printf("opening pcap %s\n", out.getFileName().c_str());

    // https://en.wikipedia.org/wiki/Transmission_Control_Protocol#Protocol_operation
    pcpp::TcpLayer syn = pcpp::TcpLayer(user_port, target_port);
    syn.getTcpHeader()->synFlag = 1;
    syn.getTcpHeader()->sequenceNumber = pcpp::hostToNet32(2);
    write_tcp(user_mac, target_mac, user_ip, target_ip, syn, NULL, 0);

    pcpp::TcpLayer syn_ack = pcpp::TcpLayer(target_port, user_port);
    syn_ack.getTcpHeader()->synFlag = 1;
    syn_ack.getTcpHeader()->ackFlag = 1;
    syn_ack.getTcpHeader()->ackNumber = pcpp::hostToNet32(3);
    syn_ack.getTcpHeader()->sequenceNumber = pcpp::hostToNet32(2);
    write_tcp(target_mac, user_mac, target_ip, user_ip, syn_ack, NULL, 0);

    pcpp::TcpLayer ack = pcpp::TcpLayer(user_port, target_port);
    ack.getTcpHeader()->ackFlag = 1;
    ack.getTcpHeader()->ackNumber = pcpp::hostToNet32(3);
    ack.getTcpHeader()->sequenceNumber = pcpp::hostToNet32(3);
    ack.getTcpHeader()->windowSize = std::numeric_limits<uint16_t>::max();
    write_tcp(user_mac, target_mac, user_ip, target_ip, ack, NULL, 0);
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

    pcpp::Packet packet(100);
    packet.addLayer(&ethernet_layer);
    packet.addLayer(&ipv4_layer);
    packet.addLayer(&tcp_layer);
    pcpp::PayloadLayer payload_layer(data, len);
    packet.addLayer(&payload_layer);
    packet.computeCalculateFields();
    out.writePacket(*packet.getRawPacket());
  }
  // data from user
  void write_dummy_user(const uint8_t *data, size_t data_len) {
    std::lock_guard lock(writer_gaurd);
    pcpp::TcpLayer user_tcp_layer = pcpp::TcpLayer(user_port, target_port);
    user_tcp_layer.getTcpHeader()->ackFlag = 1;
    user_tcp_layer.getTcpHeader()->ackNumber =
        pcpp::hostToNet32(target_sequence_number);
    user_tcp_layer.getTcpHeader()->sequenceNumber =
        pcpp::hostToNet32(user_sequence_number);
    user_tcp_layer.getTcpHeader()->windowSize =
        std::numeric_limits<uint16_t>::max();
    write_tcp(user_mac, target_mac, user_ip, target_ip, user_tcp_layer, data,
              data_len);

    user_sequence_number += data_len;
  }
  // data from target
  void write_dummy_target(const uint8_t *data, size_t data_len) {
    std::lock_guard lock(writer_gaurd);
    pcpp::TcpLayer target_tcp_layer = pcpp::TcpLayer(target_port, user_port);
    target_tcp_layer.getTcpHeader()->ackFlag = 1;
    target_tcp_layer.getTcpHeader()->ackNumber =
        pcpp::hostToNet32(user_sequence_number);
    target_tcp_layer.getTcpHeader()->sequenceNumber =
        pcpp::hostToNet32(target_sequence_number);
    target_tcp_layer.getTcpHeader()->windowSize =
        std::numeric_limits<uint16_t>::max();

    write_tcp(target_mac, user_mac, target_ip, user_ip, target_tcp_layer, data,
              data_len);
    target_sequence_number += data_len;
  }
};
