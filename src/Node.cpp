#include "Node.h"

using namespace bicycle;

void Node::setName( const std::string& name ) {
  _name = name;
}

void Node::setDesc( const std::string& desc ) {
  _desc = desc;
}

void Node::setEdges( const std::map<std::string, Edge>& edges ) {
  _edges = edges;
}

auto Node::getEdges() const -> const std::map<std::string, Edge>& {
  return _edges;
}

void Node::setEvent( const Event& event ) {
  _event = event;
}

void Node::run() {
  _event.run();
}
