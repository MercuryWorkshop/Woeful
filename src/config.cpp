#include "server.h"

WoefulConfig global_conf;

WoefulConfig *get_conf() { return &global_conf; }
void set_conf(WoefulConfig conf) { global_conf = conf; }

tl::expected<WoefulConfig, std::string> read_config(char *filename) {
  tinyxml2::XMLDocument doc;
  auto res = doc.LoadFile(filename);
  if (res != tinyxml2::XML_SUCCESS)
    return tl::unexpected("Failed to get file.");

  auto root = doc.FirstChildElement("config");
  if (root == NULL) {
    return tl::unexpected("Failed to find root.");
  }

  WoefulConfig ret;
  auto child = root->FirstChildElement();
  while (child != NULL) {
    if (strcmp(child->Name(), "port") == 0) {
      auto attribute = child->FindAttribute("port_number");
      if (attribute == NULL) {
        return tl::unexpected("Failed to find port_number.");
      }
      unsigned int port_number;
      if (attribute->QueryUnsignedValue(&port_number) !=
          tinyxml2::XML_SUCCESS) {
        return tl::unexpected("Failed to parse port_number.");
      }
      ret.port_number = port_number;

      child = child->NextSiblingElement();
      continue;
    }
    if (strcmp(child->Name(), "pcap") == 0) {
      auto attribute = child->FindAttribute("pcap_capture");
      if (attribute == NULL) {
        return tl::unexpected("Failed to find pcap_capture.");
      }
      bool pcap_capture;
      if (attribute->QueryBoolValue(&pcap_capture) != tinyxml2::XML_SUCCESS) {
        return tl::unexpected("Failed to parse pcap_capture.");
      }
      ret.pcap_capture = pcap_capture;

      child = child->NextSiblingElement();
      continue;
    }

    return tl::unexpected("Unknown element.");
  }

  return ret;
}
