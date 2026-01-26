#include "Blackboard.h"

auto BlackboardRegistry::getInstance() -> BlackboardRegistry& {
  static BlackboardRegistry registry;
  return registry;
}

void BlackboardRegistry::add( const std::string& name, const std::shared_ptr<Blackboard>& bb ){
  auto& reg = getInstance();
  reg[name] = bb;
}
