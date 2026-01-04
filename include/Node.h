#pragma once
#include <vector>
#include <string>
#include "Event.h"
#include <yaml-cpp/yaml.h>
#include <optional>
#include <map>
#include <vector>
#include <memory>
#include "Edge.h"

struct Edge;

class Node {
  public:
    Node(const YAML::Node& cfg);
    void run();  // traverses event tree
    auto isEdgeOpen( const std::string& neighbor ) const -> bool;
    auto getEdge( const std::string& neighbor ) -> std::optional<Edge>;
    auto getEdges() const -> const std::map<std::string, Edge>&;

  private:
    std::map<std::string, Edge> _edges{};
    Event _eventRoot;
    std::string _name;
    std::string _description;
};
