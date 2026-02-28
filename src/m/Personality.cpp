#include "Personality.h"

void ActArg::setBlackboard( std::shared_ptr<Blackboard> bb ) {
  _bb = bb;
}

void ActArg::setPortSet( std::shared_ptr<PortSet> ps ) {
  _ps = ps;
}

auto ActArg::getBlackboard() -> const std::shared_ptr<Blackboard>& {
  return _bb;
}

auto ActArg::getPortSet() -> const std::shared_ptr<PortSet>& {
  return _ps;
}

auto ActionRegistry::get( const std::string& name ) -> ActionPtr {
  auto& reg = getInstance();
  try {
    return reg.at( name );
  }
  catch ( const std::out_of_range& e ) {
    bicycle::die( "Action Registry hasn't mapped anything yet to key " + name + "." );
  }
  return nullptr;
}
// Allows you to more easily make an event mapping
void ActionRegistry::add( const ActionRegistry::value_type& val ) {
  if ( val.second == nullptr ) {
    bicycle::die( "ActionRegistry::add(): key \'" + val.first + "\' is paired with a nullptr value." );
  }
  auto& reg = getInstance();
  reg.insert( val );
}

auto ActionRegistry::getInstance() -> ActionRegistry& {
  static ActionRegistry registry;
  return registry;
}

void PortTypeRegistry::add( const PortTypeRegistry::value_type& val ) {
  auto& reg = getInstance();
  reg.insert( val );
}

auto PortTypeRegistry::get( const std::string& key ) -> const std::type_index& {
  auto& reg = getInstance();
  return reg.at( key );
}

auto PortTypeRegistry::getInstance() -> PortTypeRegistry& {
  static PortTypeRegistry ps;
  return ps;
}

// So i can do a flexibe array of strings at the end of my struct, but i need a way to count them. I could be dumb and manually count them. But I don't want to do that. 
Action::Action( ActFunc f, std::vector<BbKey>& ports ) : f(f) {
  /* Register input and output ports */
  for ( const auto& portName : ports ) {
    try {
      _portSet->insert( std::make_pair( portName, PortTypeRegistry::get( portName ) ) );
    }
    catch ( const std::out_of_range& e ) {
      bicycle::die( "PortTypeRegistry has no definition for " + portName + " yet.\n" + e.what() );
    }
  }
  // Don't forget to register this Action if you want to use it.
}

auto Action::getPortSet() -> std::shared_ptr<PortSet> {
  return _portSet;
}

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
      auto treeNode = YAML::LoadFile( TREE_DIR + treeName.as<std::string>() + ".yml" );
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

void ActionNode::validateBlackboard() {
  auto& ps = _arg.getPortSet();
  auto& bb = _arg.getBlackboard();

  // If only one of the two pointers is null, throw error.
  if ( ( ps != nullptr ) != ( bb != nullptr ) ) {
    if ( ps == nullptr ) {
      throw std::runtime_error( "Port set is null." );
    }
    else {
      throw std::runtime_error( "Blackboard pointer is null." );
    }
  }
  // If both are null, ignore.
  else if ( ps == nullptr  && bb == nullptr ) {
    // Innocuous nullptrs when function's not supposed to access anything.
    return;
  }

  // Ensure blackboard has both the current required key and the correct type of value.
  for ( const auto& [k, v] : *ps ) {
    // Ensure blackboard has the current key.
    if ( !bb->contains( k ) ) {
      throw std::runtime_error( "blackboard has no key named \'" + k + "\'." );
    }
    // Ensure blackboards value for that key is the type required by the port's rule.
    auto bbVal = bb->at( k );
    if ( v.name() != bbVal.type().name() ) {
      int i;
      auto actual = __cxxabiv1::__cxa_demangle( v.name(), nullptr, 0, &i );
      auto expect = __cxxabiv1::__cxa_demangle( bbVal.type().name(), nullptr, 0, &i );
      throw std::runtime_error( std::string("Your call to get<") + expect + ">( \"" + k + "\" ) should be get<" + actual + ">( \"" + k + "\" )." );
    }
  }
}  // ActionNode::validateBlackboard()

void SequenceNode::run() {
  ActionState state{ActionState::COMPLETE};  // in case there's nothing in _actions
  for ( auto& e : _actions ) {
    // Skip already-completed events.
    // There needs to be an easy way to repeat though without resetting.
    //if ( e->getState() == ActionState::COMPLETE ) {
      //continue;
    //}
    e->run();
    // Stop on the first event that fails.
    if ( e->getState() == ActionState::FAILED ) {
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

void SequenceNode::validateBlackboard() {
  for ( auto& a : _actions ) {
    a->validateBlackboard();
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
  ActionState state{ActionState::COMPLETE};  // init'd in case there's nothing in _actions
  for ( auto& e : _actions ) {
    // Skip already-completed events.
    //if ( e->getState() == ActionState::COMPLETE ) {
      //continue;
    //}
    e->run();
    // Stop on the first event that succeeds.
    if ( e->getState() == ActionState::COMPLETE ) {
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

void SequenceNode::fill( const YAML::Node& node ) {
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
    if ( quirk.priority >= _activePriority ) {
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
    throw e;
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

void Personality::validate() {
  for ( auto& [ key, quirk ] : _quirks ) {
    auto& root = quirk.tree.getRoot();
    root->validateBlackboard();
  }
}

void Tree::run() {
  auto root = getRoot();
  root->run();
}


