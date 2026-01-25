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

constexpr std::string_view ON_START{ "onStart" };
void Node::run() {
  for ( auto& e : _entities ) {
    if ( e.personality.hasTrigger( ON_START.data() ) ) {
      e.personality.trigger( ON_START.data() );
    }
  }
}
