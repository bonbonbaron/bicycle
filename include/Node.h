#pragma once

#include <string>
#include <map>

#include "Edge.h"
#include "Event.h"
#include "YmlNode.h"
#include <yaml-cpp/node/convert.h>

// TODO turn this into a class so devs can't screw around with its internals
namespace bicycle {  // prevent clash with YAML::Node
  struct Node {
      std::string name;
      std::string desc;
      std::map<std::string, Edge> edges{};
      Event event{};
  };
}

// Provide yaml-cpp library with template option for Edge's specific struct
template<>
struct YAML::convert<bicycle::Node> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, bicycle::Node& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    rhs.name = node["name"].as<std::string>();
    if ( auto desc = node["desc"] ) {
      rhs.desc = desc.as<std::string>();
    }
    rhs.edges = node["edges"].as<std::map<std::string, Edge>>();
    rhs.event = node["event"].as<Event>();
    return true;
  }
};
