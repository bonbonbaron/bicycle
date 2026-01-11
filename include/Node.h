#pragma once

#include <string>
#include <map>

#include "Edge.h"
#include "Event.h"
#include "YmlNode.h"
#include <yaml-cpp/node/convert.h>

namespace bicycle {  // prevent clash with YAML::Node
  class Node {
    public:
      void setName( const std::string& );
      void setDesc( const std::string& );
      void setEdges( const std::map<std::string, Edge>& edges );
      auto getEdges() const -> const std::map<std::string, Edge>&;
      void setEvent( const Event& event );
      void run();
    private:
      std::string _name;
      std::string _desc;
      std::map<std::string, Edge> _edges{};
      Event _event{};
  };
}

// Provide yaml-cpp library with template option for Node's specific struct
template<>
struct YAML::convert<bicycle::Node> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, bicycle::Node& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    rhs.setName( node["name"].as<std::string>() );
    if ( auto desc = node["desc"] ) {
      rhs.setDesc( desc.as<std::string>() );
    }
    rhs.setEdges( node["edges"].as<std::map<std::string, Edge>>() );
    rhs.setEvent( node["event"].as<Event>() );
    return true;
  }
};
