#pragma once
#include <map>
#include "c/InputData.h"  // TODO: Shouldn't this be m/InputData?
#include "c/Timer.h"
#include "m/Entity.h"
#include "m/Personality.h"
#include "m/Activity.h"
#include "m/Blackboard.h"

// Trigger will have a map of entities-to-personalities. We DON'T iterate through all of these every frame.
// That would be dumb. Instead, we let events (collisions, timers, and input) drive it. 
// Once a behavior starts that needs to happen on a regular basis, it's given a timer to repeat
// it at a given frequency for a specified number of reps (which can be infinite).

class TriggerRegistry : public std::map<std::string, ActCallback> {
  public:
    // Allows you to more easily make an event mapping
    static auto get( const std::string& name ) -> ActCallback;
    // Allows you to more easily make an event mapping
    static void add( const TriggerRegistry::value_type& val );

  private:
    static auto getInstance() -> TriggerRegistry&;
    TriggerRegistry() = default;
    TriggerRegistry( const TriggerRegistry& rhs ) = delete;
    TriggerRegistry& operator=( const TriggerRegistry& ) = delete;
};

// TODO wait till core Trigger takes shape before you worry about orchestrating stopping components.
class Trigger {
  public:
    static auto getInstance() -> Trigger&;
    void run();

    // Le trifecta
    static void onInput( const InputState& input );  // straightforward feeding to whatever holds context
    static void onTimer( const TimeoutMsg& timeoutMsg );  // TODO: timer ID should map to a quirk.
    static void onCollision( const int collisionType );  // TODO
  private:
    Trigger() = default;
    Trigger(const Trigger&) = delete;
    Trigger operator=(const Trigger&) = delete;
    Trigger(const Trigger&&) = delete;
    Trigger operator=(const Trigger&&) = delete;

    Entity _context{};  // The context receives inputs.
    std::map<Entity, Personality> _personalityMap{};
    // timeout ID gives entity; timeout type gives entity's quirk
    // Entities don't have to implement "onTimeout "ANIMATION_TIMEOUT" or anything basic like that.
    std::array<Entity, MAX_NUM_TIMERS> activeTimers{};
    std::array<Personality, NUM_SUPPORTED_ENTITIES> _personalities{};
    std::array<Activity, NUM_SUPPORTED_ENTITIES> _activities{};
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

    // General trigger of input, timer, and collision actions
    template<typename T>
      void onTrigger( const std::string&& stimulus, const Entity entity, T& t ) {
        // Get the entity's personality.
        auto& trigger = getInstance();
        auto personality = trigger._personalityMap.find( entity );
        if ( personality == trigger._personalityMap.end() ) {
          std::cout << "Trigger's personality map doesn't have entity " << entity << '\n';
          return;
        }

        // Get entity's triggered quirk from its personality.
        auto quirkPair = personality->second.find( stimulus );  // TODO does this need to be an orchestra instead?
        if ( quirkPair == personality->second.end() ) {
          return;
        }
        auto& quirk = quirkPair->second;

        // Get triggered action and current activity to see whether the former overrides the latter.
        // TODO if triggered action is the same as one already ongoing (do quirks need IDs to know that?), 
        auto& currActivity = _activities.at( entity );
        auto& triggeredAction = std::get<Cb<T>>( quirk.action );
        auto& bb = _blackboards.at( entity );
        if ( currActivity.state == ActionState::IN_PROGRESS || 
            currActivity.priority > quirk.priority ) {
          // TODO does this mean we call the current activity's action again and decrement its nRepsRemaining?
        }
        else {
          // Initialize and start overriding activity.
          currActivity = quirk;
        }

        // Talk to timer if this is a periodic activity.
        if ( currActivity.nRepsRemaining > 0 ) {
          --currActivity.nRepsRemaining;
          auto& timer = Timer::getInstance();
          constexpr unsigned REGULAR_TIMER_TYPE{0};
          // (30/s * 1s/1000ms)^-1
          auto timerId = timer.start( static_cast<unsigned>( 1e3 / currActivity.freq ), REGULAR_TIMER_TYPE, "REGULAR_REPEAT" );
          // TODO map timer ID to an entity, orchestra, or something so at least one action can be paused/stopped
        }
        // Call current activity's action.
        currActivity.state = triggeredAction( bb, t );
      }  // onTrigger()
    /* Input goals:
       ============
       \0. make it build (excluding things you don't need atm)
       \1. receive Input... print here (see if you can hack it to not need game data for now)
       \2. have a context... direct input to it.
       3. pretend to trigger an action on that context by getting its personality (key-quirk mapping, right?)
       */

};  // class Trigger
