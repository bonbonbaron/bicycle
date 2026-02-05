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
      
static std::mutex _nodeMut{};  // YAML-copying structures prevents uncopyable mutexes from being class members

// Classes 

// Edges are mapped by name, so there's no need for a name field.
class Edge {
  public:
    void getEndpoint();
    void setWeight( const int weight );
    auto getWeight() const -> int;
    void setEndpoint( const std::string& endpoint );
    auto getEndpoint() const -> const std::string&;
    void loadEndpoint();
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
      auto getDesc() const -> const std::string&;
      void setEdges( const std::map<std::string, Edge>& edges );
      auto getEdges() const -> const std::map<std::string, Edge>&;
      auto getEntities() const -> const std::vector<Entity>&;
      void addEntity( const Entity& entity );
      void run();
    private:
      std::string _name;
      std::string _desc;
      std::map<std::string, Edge> _edges{};
      Entity _rootEntity;  // this entity usually encapsulates other entities; think of it as a scene
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
    // Edge YAML structure should be map.
    if (!node.IsMap()) {
      return false;
    }
    // Weight (optional)
    if ( auto weight = node["weight"] ) {
      rhs.setWeight( weight.as<int>() );
    }
    // Endpoint
    rhs.setEndpoint( node["endpointFilename"].as<std::string>() );
    // Conditional (optional)
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
    // TODO delete the position portion as it no longer makes any sense in the context of Node.
    for ( const auto& e : entities ) {
      auto entityName = e.first.as<std::string>();
      YAML::Node entityNode;
      try {
        entityNode = YAML::LoadFile( ENTITY_DIR + entityName + SUFFIX.data() );
      }
      catch ( const YAML::BadFile& e ) {
        bicycle::die( "In node " + nodeName + ": for entity " + entityName + ": We couldn't find " + ENTITY_DIR + entityName + SUFFIX.data() + "." );
      }
      Entity entity;
      try {
        entity = entityNode.as<Entity>();
      } 
      catch ( const std::runtime_error& e ) {
        std::cerr << "Error for entity \'" << entityName << "\':\n";
        bicycle::die( e.what() );
      }
      if ( ! e.second.IsSequence() ) {
        bicycle::die( "In node " + nodeName + ": for entity " + entityName + ": pos node needs to be a sequence.\n" );
      }
      // Sequence of 2 integers = one instance's position
      if ( e.second[0].IsScalar() && e.second.size() == 2 ) {
        auto position = e.second.as<Position>();
        auto posPtr = std::make_shared<Position>( position );
        entity.body.setPosition( posPtr );
        rhs.addEntity( entity );
      }
      // Sequence of at least 1 position
      else if ( e.second.IsSequence() ) {
        try {
          auto positions = e.second.as<std::vector<Position>>();
          for ( auto& pos : positions ) {
            auto posPtr = std::make_shared<Position>( pos );
            entity.body.setPosition( posPtr );
            rhs.addEntity( entity );
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
