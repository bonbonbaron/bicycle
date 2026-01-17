#include "Personality.h"

static auto extractNode( const YAML::Node& node ) -> std::shared_ptr<ActionNode> {
  std::shared_ptr<ActionNode> actionNode;
  if ( auto seq = node["seq"] ) {
    actionNode = makeShared<SequenceNode>( seq );
  }
  else if ( auto fall = node["fall"] ) {
    actionNode = makeShared<FallbackNode>( fall );
  }
  else if ( auto treeName = node["tree"] ) {  // graft on another tree as a sub-tree
    auto treeNode = YAML::LoadFile( TREE_DIR.data() + treeName.as<std::string>() + ".yml" );
    actionNode = treeNode.as<Tree>().getRoot();
  }
  else {
    actionNode = std::make_shared<ActionNode>( node.as<ActionNode>() );
  }
  return actionNode;
}

void ActionNode::reset() {
  setState( ActionState::READY );
}

auto ActionNode::getState() const -> ActionState {
  return _state;
}

void ActionNode::run() {
  _state = _action->f( _arg );
}

void ActionNode::setBlackboard ( const std::shared_ptr<Blackboard> bb ) {
  _arg.setBlackboard( bb );
}

void FallbackNode::run() {
  ActionState state{ActionState::SUCCESS};  // in case there's nothing in _actions
  for ( auto& e : _actions ) {
    // Skip already-completed events.
    if ( e->getState() == ActionState::SUCCESS ) {
      continue;
    }
    e->run();
    // Stop on the first event that succeeds.
    if ( e->getState() == ActionState::SUCCESS ) {
      break;
    }
  }
  setState(state);
}

void SequenceNode::run() {
  ActionState state{ActionState::SUCCESS};  // in case there's nothing in _actions
  for ( auto& e : _actions ) {
    // Skip already-completed events.
    if ( e->getState() == ActionState::SUCCESS ) {
      continue;
    }
    e->run();
    // Stop on the first event that fails.
    if ( e->getState() != ActionState::SUCCESS ) {
      break;
    }
  }
  setState(state);
}

void SequenceNode::setBlackboard ( const std::shared_ptr<Blackboard> bb ) {
  for ( auto& a : _actions ) {
    a->setBlackboard( bb );
  }
}

// Inheritors of ActionNode will implement their ports.
void ActionNode::setAction( const std::shared_ptr<Action>& action ) {
  _action = action;
  _arg.setPortSet( _action->getPortSet() ); 
}

void SequenceNode::reset() {
  for ( auto& e : _actions ) {
    e->reset();
  }
  setState( ActionState::READY );
}

auto Tree::getRoot() ->  const std::shared_ptr<ActionNode> {
  return _root;
}

void SequenceNode::addActionNode( const std::shared_ptr<ActionNode>& actionNode ) {
  _actions.push_back( actionNode );
}

void SequenceNode::fillSequenceWithActionPtrs( const YAML::Node& node ) {
  for ( const auto& n : node ) {
    auto actionNode = ActionNode::extractNode( n );
    addActionNode( actionNode );
  }
}

void Tree::setRoot( const std::shared_ptr<ActionNode> actionNode ) {
  _root = actionNode;
}

void Tree::run() {

}


