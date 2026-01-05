#pragma once

#include <vector>
#include <string>
#include <yaml-cpp/yaml.h>
#include <optional>
#include <map>
#include <vector>
#include <memory>

#include "Edge.h"
#include "Event.h"

struct Edge;

class Node {
  public:
    Node(const YAML::Node& cfg);
    void run();  // traverses event tree
    auto isEdgeOpen( const std::string& neighbor ) const -> bool;
    void openEdge ( const std::string& neighbor );
    void closeEdge ( const std::string& neighbor );
    auto getEdge( const std::string& neighbor ) const -> std::optional<Edge>;
    auto getEdges() const -> const std::map<std::string, Edge>&;

  private:
    std::map<std::string, Edge> _edges{};
    Event _eventRoot{};
    std::string _name;
    std::string _desc;
};
