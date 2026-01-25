#include "Node.h"

using namespace bicycle;

void Node::setName( const std::string& name ) {
  _name = name;
}

auto Node::getName() const -> const std::string& {
  return _name;
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

auto Node::getEntities() const -> const std::vector<Entity>& {
  return _entities;
}

auto Node::setEntities( const std::vector<Entity>& entities ) {
  _entities = entities;
}

auto Node::setEntities( const std::vector<Entity>&& entities ) {
  _entities = entities;
}

constexpr std::string_view ON_START{ "onStart" };
void Node::run() {
  for ( auto& e : _entities ) {
    if ( e.personality.hasTrigger( ON_START.data() ) ) {
      e.personality.trigger( ON_START.data() );
    }
  }
}
