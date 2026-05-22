#include "c/Activity.h"
#include "m/Entity.h"
#include "c/Timer.h"
#include "m/Personality.h"
#ifdef SSH
#include "c/SshInput.h"
#else
#include "c/Input.h"
#endif

// HYPOTHESIS: Entities have only three ways of being activated: input, timers, and collisions.

// ONLY inputs are context-sensitive. Collisions and timers are transitive.
void Activity::onInput( const InputState& input ) {
  auto& activity = Activity::getInstance();
  try {
    (*bb)["input"] = std::make_any<char>( input );
    personality.trigger( "onInput" );
  }
  catch ( const std::out_of_range& e ) {
    bicycle::die( "Entity with symbol \"" + body.getSymbol() + "\" doesn't have an \"onInput\" trigger." );
  }
}

void Activity::onTimer( const TimerId& timerId ) {
  personality.trigger( timerId );
}

void Activity::onCollision( const int collisionType ) {
  (*bb)["collsionType"] = collisionType;
  personality.trigger( "onCollision" );
}

#if 0 //TODO
void Activity::validate() {
  personality.validate();
}
#endif
