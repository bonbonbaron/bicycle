#include "m/Personality.h"

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
  _state = _action.f( _arg );
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

// Inheritors of ActionNode will implement their ports.
void ActionNode::setAction( const ActionPtr& action ) {
  _action = *action;
  _arg.setPortSet( _action.getPortSet() ); 
}


void Personality::setQuirks( const Quirks& quirks ) {
  _quirks = quirks;
}
auto Personality::hasTrigger( const QKey& key ) -> bool {
  return _quirks.find( key ) != _quirks.end();
}


