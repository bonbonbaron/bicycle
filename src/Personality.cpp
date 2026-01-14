#include "Personality.h"

auto FallbackNode::run() -> ActionState {
  ActionState state{ActionState::SUCCESS};  // in case there's nothing in _actions
  for ( auto& e : _actions ) {
    // Skip already-completed events.
    if ( e->getState() == ActionState::SUCCESS ) {
      continue;
    }
    auto state = e->run();
    // Stop on the first event that succeeds.
    if ( state == ActionState::SUCCESS ) {
      break;
    }
  }
  setState(state);
  return getState();
}

auto SequenceNode::run() -> ActionState {
  ActionState state{ActionState::SUCCESS};  // in case there's nothing in _actions
  for ( auto& e : _actions ) {
    // Skip already-completed events.
    if ( e->getState() == ActionState::SUCCESS ) {
      continue;
    }
    auto state = e->run();
    // Stop on the first event that fails.
    if ( state != ActionState::SUCCESS ) {
      break;
    }
  }
  setState(state);
  return getState();
}

void SequenceNode::reset() {
  for ( auto& e : _actions ) {
    e->reset();
  }
  setState( ActionState::READY );
}

void SequenceNode::fillSequenceWithActionPtrs( const YAML::Node& node ) {
  for ( const auto& n : node ) {
    std::shared_ptr<ActionNode> actionNode;
    if ( auto seq = n["seq"] ) {
      actionNode = makeShared<SequenceNode>( seq );
    }
    else if ( auto fall = n["fall"] ) {
      actionNode = makeShared<FallbackNode>( fall );
    }
    else {
      actionNode = std::make_shared<ActionNode>( n.as<ActionNode>() );
    }
    addActionNode( actionNode );
  }
}
