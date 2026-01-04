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
    auto isNeighborOpen( const std::string& neighbor ) const -> bool;
    auto getNeighbor( const std::string& neighbor ) -> std::optional<std::shared_ptr<Node>>;
    auto getNeighbors() const -> const std::vector<std::shared_ptr<Node>>;
    auto getEdges() const -> const std::vector<Edge>;

  private:
    std::map<std::string, Edge> _edges{};
    Event _eventRoot;
    std::string _name;
    std::string _description;
};
