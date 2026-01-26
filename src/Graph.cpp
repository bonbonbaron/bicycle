#include "Graph.h"
using namespace bicycle;

void Edge::setWeight( const int weight ) {
  _weight = weight;
}

void Edge::setEndpoint( const std::string& endpoint ) {
  _endpointFilename = endpoint;
}

void Edge::setCondition( const std::shared_ptr<Condition>& func ) {
  _condition = func;
}

auto Edge::getWeight() const -> int {
  return _weight;
}

auto Edge::getEndpoint() const -> const std::string& {
  return _endpointFilename;
}

void Edge::loadEndpoint() const {
  constexpr std::string_view NODE_DIR{ "config/node/" };
  constexpr std::string_view SUFFIX { ".yml" };
  if ( ! _condition.has_value() || (**_condition)() ) {
    auto cfg = YAML::LoadFile( NODE_DIR.data() + _endpointFilename + SUFFIX.data() );
    auto ctlr = cfg.as<bicycle::Node>();
    ctlr.run();
  }
}


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

void Node::addEntity( const Entity& entity ) {
  _entities.push_back( entity );
}

constexpr std::string_view ON_START{ "onStart" };
void Node::run() {
  for ( auto& e : _entities ) {
    if ( e.personality.hasTrigger( ON_START.data() ) ) {
      e.personality.trigger( ON_START.data() );
    }
  }
}
