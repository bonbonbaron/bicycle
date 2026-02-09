#include "Graph.h"
using namespace bicycle;

void Edge::setWeight( const int weight ) {
  _weight = weight;
}

void Edge::setEndpoint( const std::string& endpoint ) {
  _endpointFilename = endpoint;
}

auto Edge::getWeight() const -> int {
  return _weight;
}

auto Edge::getEndpoint() const -> const std::string& {
  return _endpointFilename;
}

void Edge::loadEndpoint() {
  constexpr std::string_view NODE_DIR{ "config/node/" };
  constexpr std::string_view SUFFIX { ".yml" };
  std::unique_lock<std::mutex> l(_nodeMut);
  auto nodeCfg = YAML::LoadFile( NODE_DIR.data() + _endpointFilename + SUFFIX.data() );
  auto node = nodeCfg.as<bicycle::Node>();
  node.run();
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

auto Node::getDesc() const -> const std::string& {
  return _desc;
}

void Node::setEdges( const std::map<std::string, Edge>& edges ) {
  _edges = edges;
}

auto Node::getEdges() const -> const std::map<std::string, Edge>& {
  return _edges;
}

void Node::setRootEntity( const Entity& entity ) { 
  _rootEntity = entity;
}

constexpr std::string_view ON_START{ "onStart" };
void Node::run() {
  if ( _rootEntity.personality.hasTrigger( ON_START.data() ) ) {
    _rootEntity.personality.trigger( ON_START.data() );
  }
  else {
    throw std::runtime_error( "Entity " + _rootEntity.name + " doesn't have an ON_START trigger." );
  }
}
