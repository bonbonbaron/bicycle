#include "Node.h"

Node::Node(const YAML::Node& cfg) { 
  // TODO
}

void Node::run() {
  _eventRoot.run();
}

auto Node::isNeighborOpen( const std::string& neighbor ) const -> bool {
  const auto edge = _edges.find( neighbor );
  if ( edge != _edges.end() ) {
    return edge->second.open;
  }
  return false;
}

auto Node::getNeighbor( const std::string& neighbor ) -> std::optional<std::shared_ptr<Node>> {
  auto edge = _edges.find( neighbor );
  if ( edge != _edges.end() ) {
    return { edge->second.node };
  }
  return {};
}

auto Node::getNeighbors() const -> const std::vector<std::shared_ptr<Node>> {
  std::vector<std::shared_ptr<Node>> neighbors{};
  for ( const auto& e : _edges ) {
    if ( e.second.open ) {
      neighbors.push_back( e.second.node );
    }
  }
  return neighbors;
}

auto Node::getEdges() const -> const std::vector<Edge> {
  std::vector<Edge> edges{};
  for ( const auto& e : _edges ) {
    if ( e.second.open ) {
      edges.push_back( e.second );
    }
  }
  return edges;
}

