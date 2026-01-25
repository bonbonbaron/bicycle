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
    void setCondition( const std::shared_ptr<Condition>& func );
  private:
    int _weight{};                  // e.g. Number of random battles may be proportional to travel distance.
    std::string _endpointFilename;  // Endpoint file's basename (no path or extension)
    std::optional<std::shared_ptr<Condition>> _condition{};  // e.g. "do you have at least one key?"
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
      void addEntity( const Entity& entity );

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


// ******************************
// YAML Conversions
// ******************************

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
    if ( auto condNode = node["condition"] ) {
      auto& reg = ConditionRegistry::getInstance();
      try {
        auto conditionName = condNode.as<std::string>();
        try {
          auto& it = reg.at( conditionName );
          rhs.setCondition( it );
        }
        catch ( const std::out_of_range& e ) {
          std::cerr << "Couldn't find condition " << conditionName << " in ConditionRegistry.\n";
          throw e;
        }
      }
      catch ( const YAML::Exception& e ) {
        std::cerr << "Error converting condition node to a string\n";
        throw e;
      }
    }  // if this edge is conditional
    return true;
  }
};

// Provide yaml-cpp library with template candidate for Node's specific struct
template<>
struct YAML::convert<bicycle::Node> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, bicycle::Node& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    auto nodeName = node["name"].as<std::string>();
    rhs.setName( nodeName );
    if ( auto desc = node["desc"] ) {
      rhs.setDesc( desc.as<std::string>() );
    }
    try {
      rhs.setEdges( node["edges"].as<std::map<std::string, Edge>>() );
    }
    catch ( const YAML::Exception& e ) {
      std::cerr << "Error processing YAML for node " << nodeName << "\n";
      bicycle::die( e.what() );
    }
    catch ( const std::out_of_range& e ) {
      bicycle::die( "Error processing YAML for node " + nodeName );
    }

    auto entities = node["entities"];
    if ( !entities.IsMap() ) {
      bicycle::die( "Node " + rhs.getName() + "'s entities node needs to be a map." );
    }

    // For each entity, the key is the entity name, val is position.
    for ( const auto& e : entities ) {
      auto entityName = e.first.as<std::string>();
      YAML::Node entityNode;
      try {
        entityNode = YAML::LoadFile( ENTITY_DIR + entityName + SUFFIX.data() );
      }
      catch ( const YAML::BadFile& e ) {
        bicycle::die( "In node " + nodeName + ": for entity " + entityName + ": We couldn't find " + ENTITY_DIR + entityName + SUFFIX.data() + "." );
      }
      auto entity = entityNode.as<Entity>();
      if ( ! e.second.IsSequence() ) {
        bicycle::die( "In node " + nodeName + ": for entity " + entityName + ": pos node needs to be a sequence.\n" );
      }
      // Sequence of 2 integers = one instance's position
      Entity entityCopy;
      if ( e.second[0].IsScalar() && e.second.size() == 2 ) {
        auto position = e.second.as<Position>();
        entityCopy = entity;
        entityCopy.body.setPosition( position );
        rhs.addEntity( entityCopy );
      }
      // Sequence of at least 1 position
      else if ( e.second.IsSequence() ) {
        try {
          auto positions = e.second.as<std::vector<Position>>();
          for ( auto& pos : positions ) {
            entityCopy = entity;
            entityCopy.body.setPosition( pos );
            rhs.addEntity( entityCopy );
          }
        }
        catch ( const YAML::Exception& e ) {
          bicycle::die( "In node " + nodeName + ": for entity " + entityName + ": pos error: " + e.what() );
        }
      }
    }

    return true;
  }
};
