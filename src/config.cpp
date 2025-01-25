#include "server.h"
#include <cstring>

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

  auto block_type_attr = root->Attribute("block_type");
  if (block_type_attr != NULL) {
    if (strcmp(block_type_attr, "black") == 0) {
      ret.block_type = WoefulConfig::BLOCK_BLACK;
    } else if (strcmp(block_type_attr, "white") == 0) {
      ret.block_type = WoefulConfig::BLOCK_WHITE;
    } else if (strcmp(block_type_attr, "none") == 0) {
      ret.block_type = WoefulConfig::BLOCK_NONE;
    } else {
      return tl::unexpected("Attribute block_type has invalid value");
    }
  }

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

    if (strcmp(child->Name(), "list_port") == 0) {
      auto attribute = child->FindAttribute("port");
      if (attribute == NULL) {
        return tl::unexpected("Failed to find port.");
      }
      unsigned int port_number;
      if (attribute->QueryUnsignedValue(&port_number) !=
          tinyxml2::XML_SUCCESS) {
        return tl::unexpected("Failed to parse port_number.");
      }
      ret.block_members.push_back(WoefulConfig::Member(port_number));

      child = child->NextSiblingElement();
      continue;
    }

    if (strcmp(child->Name(), "list_hostname") == 0) {
      auto attribute = child->FindAttribute("hostname");
      if (attribute == NULL) {
        return tl::unexpected("Failed to find hostname.");
      }
      ret.block_members.push_back(WoefulConfig::Member(attribute->Value()));

      child = child->NextSiblingElement();
      continue;
    }

    return tl::unexpected("Unknown element.");
  }

  return ret;
}
