#include "c/Trigger.h"
#include "m/Entity.h"
#include "c/Timer.h"
#include "Constants.h"

 /* "I am the way... Nobody comes to the Father except through me." -John 14:6 
  *
  * Trigger is the gateway through which every signal must pass in the game engine.
  * This is designed to correlate tasks with each other.
  */

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
  // Route input through top window so the appropriate entity or window responds.
  currWindow->onInput( input );
}

void Trigger::onTimer( const Timeout& timeout ) {
  onTrigger( timeout.entity, timeout );
}

void Trigger::onCollision( const Collision& collision ) {
  onTrigger( collision.lhs, collision );
}

auto Trigger::getPersonality( Entity entity ) -> Personality {
  return _personalities.at( entity );
}

auto Trigger::getBlackboard( Entity entity ) -> Blackboard {
  return _blackboards.at( entity );
}

void Trigger::setPersonality( Entity entity, const Personality& personality ) {
  _personalities.at( entity ) = personality;
}
