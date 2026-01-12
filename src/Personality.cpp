#include "Personality.h"

auto FallbackNode::run() -> EventState {
  EventState state{EventState::SUCCESS};  // in case there's nothing in _events
  for ( auto& e : _events ) {
    // Skip already-completed events.
    if ( e->getState() == EventState::SUCCESS ) {
      continue;
    }
    auto state = e->run();
    // Stop on the first event that succeeds.
    if ( state == EventState::SUCCESS ) {
      break;
    }
  }
  setState(state);
  return getState();
}

void FallbackNode::reset() {
  for ( auto& e : _events ) {
    e->reset();
  }
  setState( EventState::READY );
}

auto SequenceNode::run() -> EventState {
  EventState state{EventState::SUCCESS};  // in case there's nothing in _events
  for ( auto& e : _events ) {
    // Skip already-completed events.
    if ( e->getState() == EventState::SUCCESS ) {
      continue;
    }
    auto state = e->run();
    // Stop on the first event that fails.
    if ( state != EventState::SUCCESS ) {
      break;
    }
  }
  setState(state);
  return getState();
}

void SequenceNode::reset() {
  for ( auto& e : _events ) {
    e->reset();
  }
  setState( EventState::READY );
}

void Sequence::fillSequenceWithActionPtrs( YAML::Node& node ) {
  for ( const auto& n : node ) {
    std::shared_ptr<Action> action;
    if ( auto seq = n["seq"] ) {
      action = makeShared<SequenceNode>( seq );
    }
    else if ( auto fall = n["fall"] ) {
      action = makeShared<FallbackNode>( fall );
    }
    else {
      action = std::make_shared<Action>( n.as<Action>() );
    }
    addAction( action );
  }
}
