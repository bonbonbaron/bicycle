#include "Edge.h"
#include <string_view>
#include <iostream>
#include "Node.h"
#include <bicycle.h>
#include "Controller.h"

#include <yaml-cpp/yaml.h>

void Edge::setWeight( const int weight ) {
  _weight = weight;
}

void Edge::setEndpoint( const std::string& endpoint ) {
  _endpointFilename = endpoint;
}

void Edge::setCondition( const std::function<bool()>& func ) {
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
  if ( ! _condition.has_value() || (*_condition)() ) {
    auto cfg = YAML::LoadFile( NODE_DIR.data() + _endpointFilename + SUFFIX.data() );
    auto& ctlr = Controller::getInstance();
    ctlr = cfg.as<bicycle::Node>();
    std::cout << "loaded " << _endpointFilename << "\n";
    ctlr.run();
  }
}

