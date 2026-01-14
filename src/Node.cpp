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

void Node::run() {
  // TODO figger it out
}

// Not sure this is the place to put these event triggers, but putting stakes in the ground here.
void Node::onInput( const int input ) {
  std::unique_lock l( _nodeMut );
  auto trigger = _onInputTriggers.find( input );
  if ( trigger != _onInputTriggers.end() ) {
    trigger->second();
  }
}

// TODO we need a way to grab the current active node to call its onTimer().
void Node::onTimer( std::string timerName ) {
  std::unique_lock l( _nodeMut );
  auto trigger = _onTimerTriggers.find( timerName );
  if ( trigger != _onTimerTriggers.end() ) {
    trigger->second();
  }
}
