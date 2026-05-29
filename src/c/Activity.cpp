#include "c/Activity.h"
#include "m/Entity.h"
#include "c/Timer.h"

auto ActionRegistry::getInstance() -> ActionRegistry& {
  static ActionRegistry reg;
  return reg;
}

auto ActionRegistry::get( const std::string& name ) -> ActCallback {
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
void ActionRegistry::add( const ActionRegistry::value_type& val ) {
  auto& reg = getInstance();
  reg.insert( val );
}


auto Activity::getInstance() -> Activity& {
  static Activity activity;
  return activity;
}


// ONLY inputs are context-sensitive. Collisions and timers are transitive.
void Activity::onInput() {
  auto& activity = getInstance();
  auto personality = activity._personalityMap.find( activity._context );
  if ( personality == activity._personalityMap.end() ) {
    std::cout << "Activity's personality map doesn't have entity " << activity._context << '\n';
    return;
  }
  auto inputQuirk = personality->second.find( "ON_INPUT" );
  if ( inputQuirk != personality->second.end() ) {
    // TODO the following TODOs should be wrapped in a common, templated function (<InputState> in this case)
    // TODO check reps remaining
    // TODO compare priority to active priority
    // TODO we need to pass in act arg 
    // TODO get entity's blackboard
    // inputQuirk->second.action();
  }
}

void Activity::onTimer( const TimerId timerId ) {
}

void Activity::onCollision( const int collisionType ) {
}

#if 0 //TODO
void Activity::validate() {
  personality.validate();
}
#endif
