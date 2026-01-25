#pragma once

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include <optional>
#include <vector>

#include <yaml-cpp/node/convert.h>
#include "Entity.h"
#include "ConditionRegistry.h"
      
static std::mutex _nodeMut{};  // There'll only ever be one node active.


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

// Provide yaml-cpp library with template candidate for Edge's specific struct
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
      auto& it = reg.at( conditionName.as<std::string>() );
      rhs.setCondition( it );
    }  // if this edge is conditional
    return true;
  }
};

namespace bicycle {  // prevent clash with YAML::Node
  class Node : public std::enable_shared_from_this<Node> {
    public:
      void setName( const std::string& );
      auto getName() const -> const std::string&;
      void setDesc( const std::string& );
      void setEdges( const std::map<std::string, Edge>& edges );
      auto getEdges() const -> const std::map<std::string, Edge>&;
      auto getEntities() const -> const std::vector<Entity>&;
      auto setEntities( const std::vector<Entity>& entities );
      auto setEntities( const std::vector<Entity>&& entities );

      void run();
      void onInput( const int input );
      void onTimer( const std::string timerName );
    private:
      std::string _name;
      std::string _desc;
      std::map<std::string, Edge> _edges{};
      std::map<unsigned char, std::function<void()>> _onInputTriggers{};
      std::map<std::string, std::function<void()>> _onTimerTriggers{};
      std::vector<Entity> _entities;
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
    auto entities = node["entities"];
    if ( !entities.IsSequence() ) {
      throw std::runtime_error( "Node " + rhs.getName() + "'s entities node needs to be a list." );
    }

    // For each entity, the key is the entity name, val is position.
    for ( const auto& e : entities ) {
      if ( !e.IsMap() ) {  
        throw std::runtime_error( "Node " + rhs.getName() + " has an entity that's not a map." );
      }
      auto entityName = e.first.as<std::string>();
      if ( e.second.IsSequence() ) {
        std::cout << entityName << "'s pos is a sequence\n";
      }
      else if (e.second.IsScalar() ) {
        std::cout << entityName << "'s pos is a scalar\n";
      }
    }

    return true;
  }
};
