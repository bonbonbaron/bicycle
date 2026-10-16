#pragma once

#include <string>
#include <cstdint>
#include "data/Map.h"
#include <memory>
#include <mutex>
#include <functional>
#include <optional>
#include <vector>

#include "Entity.h"
      
static std::mutex _nodeMut{};  // YAML-copying structures prevents uncopyable mutexes from being class members

// Classes 

// Edges are mapped by edge id (ids::Edge in gen/ids.h: names from edges = { <name> = ... } tables), so there's
// no need for a name field.
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
      using Edges = bicycle::map<Edge>;  // by ids::Edge
      void setEdges( const Edges& edges );
      auto getEdges() const -> const Edges&;
      void setRootEntity( const Entity& entity );
      void run();
    private:
      std::string _name;
      std::string _desc;
      Edges _edges{};
      Entity _rootEntity;  // this entity usually encapsulates other entities; think of it as a scene
  };  // class Node
}  // namespace bicycle
