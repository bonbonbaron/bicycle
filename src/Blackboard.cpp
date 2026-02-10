#include "Blackboard.h"

auto BlackboardRegistry::getInstance() -> BlackboardRegistry& {
  static BlackboardRegistry registry;
  return registry;
}

void BlackboardRegistry::add( const BlackboardRegistry::value_type& val ){
  auto& reg = getInstance();
  reg.insert( val );
}
