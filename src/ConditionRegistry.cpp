#include "ConditionRegistry.h"

auto ConditionRegistry::getInstance() -> ConditionRegistry& {
  static ConditionRegistry registry;
  return registry;
}

void ConditionRegistry::add( const std::string& name, const std::shared_ptr<Condition>& conditionFunc ) {
  auto& reg = getInstance();
  reg[name] = conditionFunc;
}

