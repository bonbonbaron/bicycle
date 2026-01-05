#include "Node.h"

Node::Node( Config::Node& cfg) { 
  _name = cfg.readRequired<std::string>( "name" );
  _desc = cfg.readRequired<std::string>( "desc" );
  // TODO node edges
  // TODO event tree
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

void Node::openEdge ( const std::string& neighbor ) {
  auto edge = _edges.find( neighbor );
  if ( edge != _edges.end() ) {
    edge->second.open = true;
  }
}

void Node::closeEdge ( const std::string& neighbor ) {
  auto edge = _edges.find( neighbor );
  if ( edge != _edges.end() ) {
    edge->second.open = false;
  }
}

auto Node::getEdge( const std::string& neighbor ) const -> std::optional<Edge> {
  auto edge = _edges.find( neighbor );
  if ( edge != _edges.end() ) {
    return { edge->second };
  }
  return {};
}

auto Node::getEdges() const -> const std::map<std::string, Edge>&  {
  return _edges;
}

