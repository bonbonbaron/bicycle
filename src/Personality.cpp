#include "Personality.h"

auto ActionNode::extractNode( const YAML::Node& node ) -> std::shared_ptr<ActionNode> {
  std::shared_ptr<ActionNode> actionNode;
  if ( node.IsSequence() ) {
    bicycle::die( "idk how to process sequences yet" );
  }
  else if ( node.IsMap() ) {
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
    // Otherwise, extract the node recursively.
  }
  else {  // if it's a scalar
    actionNode = std::make_shared<ActionNode>( node.as<ActionNode>() );
  }
  return actionNode;
}

void ActionNode::setState( const ActionState& state ) {
  _state = state;
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
void ActionNode::setAction( const ActionPtr& action ) {
  _action = action;
  _arg.setPortSet( _action->getPortSet() ); 
}

void SequenceNode::reset() {
  for ( auto& e : _actions ) {
    e->reset();
  }
  setState( ActionState::READY );
}
void FallbackNode::run() {
  ActionState state{ActionState::SUCCESS};  // init'd in case there's nothing in _actions
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

void Personality::setQuirks( const Quirks& quirks ) {
  _quirks = quirks;
}

void Personality::trigger( const std::string& rootKey ) {
  try {
    auto& quirk = _quirks.at( rootKey );
    if ( quirk.priority > _activePriority ) {
      _activePriority = quirk.priority;
      cancel();  // stop the current action
      // Start a looping timer if this is a recurring behavior.
      if ( quirk.freq ) {
        _timer = std::make_shared<Timer>( [&quirk](){ quirk.tree.run(); }, std::chrono::milliseconds( quirk.freq ), true );
      }
      // Otherwise, run this behavior once.
      else {
        _timer = nullptr;
        quirk.tree.run();
      }
    }  // if this quirk takes priority over the active one
  }
  catch ( const std::out_of_range& e ) {
    std::cerr << "No quirk exists for key \'" << rootKey << "\' in ";
    bicycle::die( e.what() );
  }
}

void Personality::cancel() {
  if ( _timer != nullptr ) {
    _timer->stop();
  }
}


void Personality::distributeBlackboard( std::shared_ptr<Blackboard> bb ) {
  for ( auto& [ key, quirk ] : _quirks ) {
    quirk.tree.getRoot()->setBlackboard( bb );
  }
}

auto Personality::hasTrigger( const std::string& key ) -> bool {
  return _quirks.find( key ) != _quirks.end();
}

void Tree::run() {
  auto root = getRoot();
  root->run();
}


