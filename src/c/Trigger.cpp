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

void Trigger::setPersonality( Entity entity, const Personality& personality ) {
  _personalities.at( entity ) = personality;
}

/*
 * The following are results of a triggered action.
 * Thus they enter into the latest spawned group.
 *    Motion
 *    Animation
 *    Rendering
 *    Sound
 *    Subtimers
 *
 * The only implemented of those are sub-timers and rendering.
 * So let's work with them first.
 *            rendering | subtimers
 *   start              |          
 *   stop               |         
 *   pause              |          
 *   unpause            |            
 *
 * Actually, the only way I can test these things out truly is to make Lua scripting a thing. So let's shift priorities and go down that road first.
 */
