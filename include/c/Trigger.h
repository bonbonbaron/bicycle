#pragma once
#include <map>
#include "c/InputData.h"
#include "c/Timer.h"
#include "m/World.h"
#include "m/Personality.h"

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
    static void onTimer( const TimerId timerIdx );  // TODO: timer ID should map to a quirk.
    static void onCollision( const int collisionType );  // TODO
  private:
    Trigger() = default;
    Trigger(const Trigger&) = delete;
    Trigger operator=(const Trigger&) = delete;
    Trigger(const Trigger&&) = delete;
    Trigger operator=(const Trigger&&) = delete;

    Entity _context{};  // The context receives inputs.
    std::map<Entity, Personality> _personalityMap{};

    /* 
     * TIMER: timer ID goes off, asks Trigger whose it was and gets the entity.
     *        timer type fishes out the quirk and triggers it. We pass in the timer information
     *        for the hell of it; whether or not the game actually uses it is none of our concern rn.
     */

    // General trigger of input, timer, and collision actions
    template<typename T>
      void onTrigger( const std::string&& stimulus, const Entity entity, T& t ) {
        // Get the entity's personality.
        auto& trigger = getInstance();
        auto personality = trigger._personalityMap.find( entity );
        if ( personality == trigger._personalityMap.end() ) {
          std::cout << "Trigger's personality map doesn't have entity " << trigger._context << '\n';
          return;
        }
        // Get the triggered quirk.
        auto quirk = personality->second.find( stimulus );
        if ( quirk == personality->second.end() ) {
          return;
        }
        // Get triggered action and current activity to see whether the former overrides the latter.
        auto& triggeredAction = std::get<Cb<T>>( quirk->second.action );
        auto& currActivity = World::get<Activity>( entity );
        auto& bb = World::get<Blackboard>( entity );
        if ( currActivity.state == ActionState::IN_PROGRESS || 
            currActivity.priority > triggeredAction.priority ) {
          // TODO does this mean we call the current activity's callback again and decrement its nRepsRemaining?
        }
        else {
          // Initialize and start overriding activity.
          currActivity = quirk;
        }

        // Talk to timer if this is a periodic activity.
        if ( currActivity.nRepsRemaining > 0 ) {
          --currActivity.nRepsRemaining;
          auto& timer = Timer::getInstance();
          // (30/s * 1s/1000ms)^-1
          timer.start( 1e3 / currActivity.freq, entity, { entity, "REGULAR_REPEAT" } );
        }
        // Call current activity's callback.
        currActivity.state = currActivity.callback( bb, t );
        

        // auto& currActivity =
        // TODO check reps remaining
        // TODO compare priority to active priority
        // TODO we need to pass in act arg 
        // TODO get entity's blackboard
        // inputQuirk->second.action();
      }
    /* Input goals:
       ============
       \0. make it build (excluding things you don't need atm)
       \1. receive Input... print here (see if you can hack it to not need game data for now)
       2. have a context... direct input to it.
       3. pretend to trigger an action on that context by getting its personality (key-quirk mapping, right?)
       */

};  // class Trigger
