#pragma once
#include <functional>
#include <string>
#include <map>
#include <cursesw.h>
#include <yaml-cpp/node/convert.h>

#include "EventRegistry.h"

struct Event {
  EventState state{ EventState::READY };
  std::function<EventState()> run{};
};



// Provide yaml-cpp library with template option for Edge's specific struct
template<>
struct YAML::convert<Event> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Event& rhs) {
    if (!node.IsScalar()) {
      return false;
    }
    auto eventName = node.as<std::string>();
    auto& reg = EventRegistry::get();
    auto it = reg.find( eventName );
    if ( it == reg.end() ) {
      std::cerr << "Event " << eventName << " not found in EventRegistry instance. Exiting...\n";
      endwin();
      exit(1);
    }
    rhs.run = it->second;
      
    return true;
  }
};

