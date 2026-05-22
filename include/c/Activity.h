#pragma once
#include <map>
#define SSH
#include "c/InputData.h"
#include "c/Timer.h"
#include "m/World.h"

// TODO wait till core Activity takes shape before you worry about orchestrating stopping components.
class Activity {
  public:
    static auto getInstance() -> Activity&;
    void run();

    // Le trifecta
    static void onInput( const InputState& input );  // straightforward feeding to entities that can handle input.
    static void onTimer( const TimerId timerIdx );  // TODO: timer ID should map to a quirk.
    static void onCollision( const int collisionType );  // TODO
  private:
    Activity() = default;
    Activity(const Activity&) = delete;
    Activity operator=(const Activity&) = delete;
    Activity(const Activity&&) = delete;
    Activity operator=(const Activity&&) = delete;

    /* Input goals:
       ============
       \0. make it build (excluding things you don't need atm)
       1. receive Input... print here (see if you can hack it to not need game data for now)
       2. have a context... pretend like input is going to context
       3. pretend to trigger an action on that context by getting its personality (key-quirk mapping, right?)
    */

};  // class Activity
