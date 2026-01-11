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

// Not sure this is the place to put these event triggers, but putting stakes in the ground here.
void Node::onInput( const int input ) {
  auto trigger = _onInputTriggers.find( input );
  if ( trigger != _onInputTriggers.end() ) {
    trigger->second();
  }
}

void Node::onTimer( const int input ) {
  auto trigger = _onInputTriggers.find( input );
  if ( trigger != _onTimerTriggers.end() ) {
    trigger->second();
  }
}
