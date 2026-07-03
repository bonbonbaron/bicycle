#include "c/Trigger.h"
#include "m/Entity.h"
#include "c/Timer.h"
#include "Constants.h"


auto Trigger::getInstance() -> Trigger& {
  static Trigger trigger;
  return trigger;
}


// ONLY inputs are context-sensitive. Collisions and timers are transitive.
void Trigger::onInput( const InputState& input ) {
  // std::cout << "Triggered input response for input " << input.currKeysPressed << '\n';
  // If top window has sub-entities, get its inner context.
  const auto& wm = WindowManager::getInstance();
  const auto currWindow = wm.back();
  assert( currWindow != nullptr );
  currWindow->onInput( input );
  // TODO the following TODOs should be wrapped in a common, templated function (<InputState> in this case)
  // TODO check reps remaining
  // TODO compare priority to active priority
  // TODO we need to pass in act arg 
  // TODO get entity's blackboard
  // inputQuirk->second.action();
}

void Trigger::onTimer( const TimeoutMsg& timeoutMsg ) {
  std::cout << "Triggered timeout response for timer ID " << timeoutMsg.id << ", type " << timeoutMsg.type << ", val " << timeoutMsg.val << '\n';
}

void Trigger::onCollision( const int collisionType ) {
}

auto Trigger::getPersonality( Entity entity ) -> Personality {
  return _personalities.at( entity );
}

auto Trigger::getBlackboard( Entity entity ) -> Blackboard {
  return _blackboards.at( entity );
}

void Trigger::setPersonality( Entity entity, const Personality& personality ) {
  std::cout << "setting personality for entity " << entity << "\n";
  _personalities.at( entity ) = personality;
}
