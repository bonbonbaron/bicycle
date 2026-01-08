#pragma once

#include <vector>
#include <string>
#include <optional>
#include <map>
#include <vector>
#include <memory>

#include "Edge.h"
#include "Event.h"
#include "YmlNode.h"

class Node {
  public:
    Node( YmlNode& cfg);
    void run();  // traverses event tree
    auto isEdgeOpen( const std::string& neighbor ) const -> bool;
    void openEdge ( const std::string& neighbor );
    void closeEdge ( const std::string& neighbor );
    auto getEdge( const std::string& neighbor ) const -> std::optional<Edge>;
    auto getEdges() const -> const std::map<std::string, Edge>&;

  private:
    // Design how edges are going to rely on game data... use optional openif: <some-condition>
    std::map<std::string, Edge> _edges{};
    Event _eventRoot{};
    std::string _name;
    std::string _desc;
};
