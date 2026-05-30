#include "c/Trigger.h"
#include "m/Entity.h"
#include "c/Timer.h"

auto TriggerRegistry::getInstance() -> TriggerRegistry& {
  static TriggerRegistry reg;
  return reg;
}

auto TriggerRegistry::get( const std::string& name ) -> ActCallback {
  auto& reg = getInstance();
  try {
    return reg.at( name );
  }
  catch ( const std::out_of_range& e ) {
    bicycle::die( "Action Registry hasn't mapped anything yet to key " + name + "." );
  }
  return {};
}

// Allows you to more easily make an event mapping
void TriggerRegistry::add( const TriggerRegistry::value_type& val ) {
  auto& reg = getInstance();
  reg.insert( val );
}


auto Trigger::getInstance() -> Trigger& {
  static Trigger activity;
  return activity;
}


// ONLY inputs are context-sensitive. Collisions and timers are transitive.
void Trigger::onInput( const InputState& input ) {
    std::cout << "Triggered input response for input " << input.currKeysPressed << '\n';
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

#if 0 //TODO
void Trigger::validate() {
  personality.validate();
}
#endif
