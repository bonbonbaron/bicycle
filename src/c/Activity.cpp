#include "c/Activity.h"
#include "m/Entity.h"
#include "c/Timer.h"
#include "m/Personality.h"

auto Activity::getInstance() -> Activity& {
  static Activity activity;
  return activity;
}


// ONLY inputs are context-sensitive. Collisions and timers are transitive.
void Activity::onInput() {
  auto& activity = getInstance();
  auto entityPersonalityPair = activity._personalityMap.find( activity._context );
  if ( entityPersonalityPair == activity._personalityMap.end() ) {
    std::cout << "Activity's personality map doesn't have entity " << activity._context << '\n';
    return;
  }
  if ( entityPersonalityPair->second.hasQuirk( "ON_INPUT" ) ) {
    auto& inputQuirk = second.getQuirk( "ON_INPUT" );

    // Choices:
    //  1. make auto-attached onInput() function at entity level that lets us pass in input state
    //  2. force every onInput action to get the instance of Input and grab its state, making passage of Input useless
    //        But wait. If I want to map inputs via yaml, then need I write any code for each input at all?
    //        The only code I ought to write is the simple response to the button press, e.g. move().
    //        This code can either check the state of Input class, or it can auto-act (e.g. punch).
    //  3. attach input as a blackboard item, which would be insanely dumb
    //  4. Better idea:If every action is accounted for, then give personality a sub-mapping of...
    //        input triggers      -> input actions
    //        collision triggers  -> collision actions
    //        timer triggers      -> timer actions
    //
    //      Then each type can have its own function signature to allow for easier passage of relevant info.
    //      Collisions need to know who bumped into whom, their types, et.c
    //      While timers need to know what timer went off (ID and type).
    //      Input needs to know the state of the input world without forcing direct retrieval.
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
