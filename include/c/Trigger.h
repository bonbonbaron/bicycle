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

    // General trigger of input, timer, and collision actions
    template<typename T>
      auto& onTrigger( const std::string&& stimulus ) -> ActionState {
        auto inputQuirk = personality->second.find( stimulus );
        if ( inputQuirk != personality->second.end() ) {
          // First, we need to see what the entity's active quirk is, if any.

          auto& action = std::get<Cb<T>>( inputQuirk->second.action );
          
          // TODO the following TODOs should be wrapped in a common, templated function (<InputState> in this case)
          // TODO check reps remaining
          // TODO compare priority to active priority
          // TODO we need to pass in act arg 
          // TODO get entity's blackboard
          // inputQuirk->second.action();
        /* Input goals:
           ============
           \0. make it build (excluding things you don't need atm)
           \1. receive Input... print here (see if you can hack it to not need game data for now)
           2. have a context... direct input to it.
           3. pretend to trigger an action on that context by getting its personality (key-quirk mapping, right?)
           */

      };  // class Trigger
