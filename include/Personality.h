#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <variant>

#include "EventRegistry.h"

enum class ActionState { READY, FAILED, IN_PROGRESS, SUCCESS };

using Action = std::function<ActionState()>;
using BehaviorTree = Action;

// Personality is the behavior of an entity.
struct Quirk {
  BehaviorTree behaviorTree{}
  unsigned priority{};  // higher values take precedence
}

using TriggerMap = std::map< std::string, Quirk >;

class Personality {
  public:
  private:
};


//TODO yaml
