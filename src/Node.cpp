#include "Node.h"

Node::Node(const YAML::Node& cfg) { 
  // TODO
}

void Node::run() {
  _eventRoot.run();
}

auto Node::isEdgeOpen( const std::string& neighbor ) const -> bool {
  const auto edge = _edges.find( neighbor );
  if ( edge != _edges.end() ) {
    return edge->second.open;
  }
  return false;
}

auto Node::getEdge( const std::string& neighbor ) -> std::optional<Edge> {
  auto edge = _edges.find( neighbor );
  if ( edge != _edges.end() ) {
    return { edge->second };
  }
  return {};
}

auto Node::getEdges() const -> const std::map<std::string, Edge>&  {
  return _edges;
}

