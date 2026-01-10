#pragma once
#include <memory>
#include <functional>
#include <optional>
#include <cursesw.h>
#include <iostream>

#include <yaml-cpp/node/convert.h>

#include "ConditionRegistry.h"

// Edges are mapped by name, so there's no need for a name field.
class Edge {
  public:
    void getEndpoint();
    void setWeight( const int weight );
    auto getWeight() const -> int;
    void setEndpoint( const std::string& endpoint );
    auto getEndpoint() const -> const std::string&;
    void loadEndpoint() const;
    void setCondition( const std::function<bool()>& func );
  private:
    int _weight{};                  // e.g. Number of random battles may be proportional to travel distance.
    std::string _endpointFilename;  // Endpoint file's basename (no path or extension)
    std::optional<std::function<bool()>> _condition{};  // e.g. "do you have at least one key?"
};

// Provide yaml-cpp library with template option for Edge's specific struct
template<>
struct YAML::convert<Edge> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Edge& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    if ( auto weight = node["weight"] ) {
      rhs.setWeight( weight.as<int>() );
    }
    rhs.setEndpoint( node["endpointFilename"].as<std::string>() );
    // If this edge is conditional
    if ( auto conditionName = node["condition"] ) {
      
      auto& reg = ConditionRegistry::get();
      auto it = reg.find( conditionName.as<std::string>() );
      if ( it == reg.end() ) {
        std::cerr << "Condition " << conditionName.as<std::string>() << " not found in ConditionRegistry instance. Exiting...\n";
        endwin();
        exit(1);
      }
      rhs.setCondition( it->second );
    }  // if this edge is conditional
    return true;
  }
};
