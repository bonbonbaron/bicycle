#include "Action.h"

void ActArg::setBlackboard( std::shared_ptr<Blackboard> bb ) {
  _bb = bb;
}

void ActArg::setPortSet( std::shared_ptr<PortSet> ps ) {
  _ps = ps;
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
void ActionRegistry::add( const std::string& name, const ActionPtr& action ) {
  if ( action == nullptr ) {
    bicycle::die( "ActionRegistry::set(): ActionPtr " + name + " is null!" );
  }
  auto& reg = getInstance();
  reg.insert( std::pair( name, action ) );
}

auto ActionRegistry::getInstance() -> ActionRegistry& {
  static ActionRegistry registry;
  return registry;
}

void PortTypeRegistry::add( const std::string&& key, const std::type_index& val ) {
  auto& reg = getInstance();
  reg.insert( std::pair( key, val ) );
}

auto PortTypeRegistry::get( const std::string& key ) -> const std::type_index& {
  auto& reg = getInstance();
  return reg.at( key );
}

auto PortTypeRegistry::getInstance() -> PortTypeRegistry& {
  static PortTypeRegistry ps;
  return ps;
}

Action::Action( const std::string& name, ActFunc f, std::vector<BbKey>&& ports ) : f(f) {
  /* Register input and output ports */
  for ( const auto& portName : ports ) {
    try {
      _portSet->insert( std::make_pair( portName, PortTypeRegistry::get( portName ) ) );
    }
    catch ( const std::out_of_range& e ) {
      std::cerr << "PortTypeRegistry has no definition for " << portName << " yet.\n";
      bicycle::die( e.what() );
    }
  }
  // Don't forget to register this Action if you want to use it.
}

auto Action::getPortSet() -> std::shared_ptr<PortSet> {
  return _portSet;
}
