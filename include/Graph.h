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
  private:
    int _weight{};                  // e.g. Number of random battles may be proportional to travel distance.
    std::string _endpointFilename;  // Endpoint file's basename (no path or extension)
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
      void setRootEntity( const Entity& entity );
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

    auto entityName = node["entity"].as<std::string>();

    YAML::Node entityNode;
    try {
      entityNode = YAML::LoadFile( ENTITY_DIR + entityName + SUFFIX.data() );
    }
    catch ( const YAML::BadFile& e ) {
      bicycle::die( "In node " + nodeName + ": for entity " + entityName + ": We couldn't find " + ENTITY_DIR + entityName + SUFFIX.data() + "." );
    }
    try {
      Entity entity = entityNode.as<Entity>();
      rhs.setRootEntity( entity );
    } 
    catch ( const std::runtime_error& e ) {
      std::cerr << "Error for entity \'" << entityName << "\':\n";
      bicycle::die( e.what() );
    }

    return true;
  }
};
