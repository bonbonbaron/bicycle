#pragma once

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <functional>

#include "Edge.h"
#include "Event.h"
#include "YmlNode.h"
#include <yaml-cpp/node/convert.h>
      
static std::mutex _nodeMut{};  // There'll only ever be one node active.

namespace bicycle {  // prevent clash with YAML::Node
  class Node : public std::enable_shared_from_this<Node> {
    public:
      void setName( const std::string& );
      void setDesc( const std::string& );
      void setEdges( const std::map<std::string, Edge>& edges );
      auto getEdges() const -> const std::map<std::string, Edge>&;
      void setEvent( const Event& event );
      void run();
      void onInput( const int input );
      void onTimer( const std::string timerName );
    private:
      std::string _name;
      std::string _desc;
      std::map<std::string, Edge> _edges{};
      Event _event{};
      std::map<unsigned char, std::function<void()>> _onInputTriggers{};
      std::map<std::string, std::function<void()>> _onTimerTriggers{};
  };  // class Node
}  // namespace bicycle

// Provide yaml-cpp library with template candidate for Node's specific struct
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
