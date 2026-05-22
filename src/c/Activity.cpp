#include "c/Activity.h"
#include "m/Entity.h"
#include "c/Timer.h"
#include "m/Personality.h"

// ONLY inputs are context-sensitive. Collisions and timers are transitive.
void Activity::onInput( const InputState& input ) {
  auto& activity = Activity::getInstance();

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
