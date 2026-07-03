#pragma once
#include <map>
#include "c/InputData.h"  // TODO: Shouldn't this be m/InputData?
#include "c/Timer.h"
#include "m/Entity.h"
#include "m/Personality.h"
// #include "m/Activity.h"
#include "m/Blackboard.h"
#include "c/WindowManager.h"

// Wait,does input need to handle things differently from timer & collision?

// Trigger will have a map of entities-to-personalities. We DON'T iterate through all of these every frame.
// That would be dumb. Instead, we let events (collisions, timers, and input) drive it. 
// Once a behavior starts that needs to happen on a regular basis, it's given a timer to repeat
// it at a given frequency for a specified number of reps (which can be infinite).

// TODO wait till core Trigger takes shape before you worry about orchestrating stopping components.
class Trigger {
  public:
    static auto getInstance() -> Trigger&;

    // Le trifecta
    static void onInput( const InputState& input );  // straightforward feeding to whatever holds context
    static void onTimer( const TimeoutMsg& timeoutMsg );  // TODO: timer ID should map to a quirk.
    static void onCollision( const int collisionType );  // TODO

    // General trigger of input, timer, and collision actions
    template<typename T>
      static void onTrigger( const Entity entity, T& t ) {
        // Get the entity's personality.
        auto& trigger = getInstance();
        auto personality = trigger.getPersonality( entity );
        auto quirk = personality[ getTypeTag<T>() ];
        // Get triggered action and current activity to see whether the former overrides the latter.
        auto& triggeredAction = std::get<Cb<T>>( quirk );
        // auto& bb = trigger.getBlackboard( entity );   // TODO leave this out until absolutely necessary
        triggeredAction( entity, t );         // Call current activity's action.
      } // onTrigger()

    auto getPersonality( Entity entity ) -> Personality;
    void setPersonality( Entity entity, const Personality& personality );
    auto getBlackboard( Entity entity ) -> Blackboard;
    // auto getActiveTimer( Entity entity ) -> Blackboard;

    // TODO whatever the above needs to be static, make accessor here.
  private:
    Trigger() = default;
    Trigger(const Trigger&) = delete;
    Trigger operator=(const Trigger&) = delete;
    Trigger(const Trigger&&) = delete;
    Trigger operator=(const Trigger&&) = delete;

    std::shared_ptr<Window> _context{};  // The context receives inputs.
    // timeout ID gives entity; timeout type gives entity's quirk
    // Entities don't have to implement "onTimeout "ANIMATION_TIMEOUT" or anything basic like that.
    std::array<Entity, MAX_NUM_TIMERS> activeTimers{};
    std::array<Personality, NUM_SUPPORTED_ENTITIES> _personalities{};
    // std::array<Activity, NUM_SUPPORTED_ENTITIES> _activities{};  // TODO this'll be overwritten with Orchestra
    std::array<Blackboard, NUM_SUPPORTED_ENTITIES> _blackboards{};
    /* 
     * TIMER: timer ID goes off, asks Trigger whose it was and gets the entity.
     *        timer type fishes out the quirk and triggers it. We pass in the timer information
     *        for the hell of it; whether or not the game actually uses it is none of our concern rn.
     */
    /*
     * But if i want timer -> animation to... 
     *  1. not stop the larger action that animation belongs to (such as walking and thinking)
     *  2. be pausable/stoppable along with the larger action
     *
     *  ... then we need to rethink quirks.
     *  Quirks assume one piece of action only and inhibits multiple timers from triggering concurrent activities.
     *  Maybe quirks need ports for various signals:
     *    timerAnimType   -> animation sub-activity
     *    timerMotionType -> motion sub-activity
     *    timerCustomType -> thinking sub-activity
     *
     *  Let's say this group has priority 4. What happens if a collision prio 5 triggers in the middle of it?
     *
     *  Notice this whole problem is all about timers.
     *  In the above ports idea, would there ever be a port for anything other than a timer?
     *  Collision is concerned a root-level trigger as is input.
     *  So I believe not.
     *  But what if a timer of bigger priority happens? Can that be a root-level trigger overthrowing this overarching activity?
     *  The Trigger::onTimer() interface needs a bit of work to distinguish between timer ports and root-level interrupts.
     *  
     */

    /* Input goals:
       ============
       \0. make it build (excluding things you don't need atm)
       \1. receive Input... print here (see if you can hack it to not need game data for now)
       2. have a top-level window... direct input to it.
       3. pretend to trigger an action on that context by getting its personality (key-quirk mapping, right?)
       */

};  // class Trigger
