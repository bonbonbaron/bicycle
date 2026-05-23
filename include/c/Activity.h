#pragma once
#include <map>
#include "c/InputData.h"
#include "c/Timer.h"
#include "m/World.h"

// Activity will have a map of entities-to-personalities. We DON'T iterate through all of these every frame.
// That would be dumb. Instead, we let events (collisions, timers, and input) drive it. 
// Once a behavior starts that needs to happen on a regular basis, it's given a timer to repeat
// it at a given frequency for a specified number of reps (which can be infinite).

// this is separate from personality. Personality is just how an entity behaves.
// Activity is what it's currently doing.
struct ActivityIner {
  Quirk currQuirk{};
  decltype(Quirk::reps) repsRemaining{};
};

// TODO wait till core Activity takes shape before you worry about orchestrating stopping components.
class Activity {
  public:
    static auto getInstance() -> Activity&;
    void run();

    // Le trifecta
    static void onInput();  // straightforward feeding to whatever holds context
    static void onTimer( const TimerId timerIdx );  // TODO: timer ID should map to a quirk.
    static void onCollision( const int collisionType );  // TODO
  private:
    Activity() = default;
    Activity(const Activity&) = delete;
    Activity operator=(const Activity&) = delete;
    Activity(const Activity&&) = delete;
    Activity operator=(const Activity&&) = delete;

    Entity _context{};  // The context receives inputs.
    std::map<Entity, Personality> _personalityMap{};

    /* Input goals:
       ============
       \0. make it build (excluding things you don't need atm)
       \1. receive Input... print here (see if you can hack it to not need game data for now)
       2. have a context... direct input to it.
       3. pretend to trigger an action on that context by getting its personality (key-quirk mapping, right?)
    */

};  // class Activity
