#pragma once
#include <unordered_map>
#include "c/InputData.h"
#include "c/Timeout.h"
#include "c/Collision.h"
#include "c/WindowManager.h"
#include "m/Position.h"
#include "c/Timer.h"
#include <lua.hpp>

 /* "I am the way... Nobody comes to the Father except through me." -John 14:6 
  *
  * Trigger is the gateway through which every signal must pass in the game engine.
  * This is designed to correlate tasks with each other.
  */

class Trigger {
  public:
    enum Action { START, STOP, PAUSE, UNPAUSE };
    enum System{ TIMER };

    static auto getInstance() -> Trigger&;
		void init();
		static int doAction( lua_State* luaState );
		auto getLuaState() -> lua_State*;

    // Le trifecta... These wrap templatized calls to onTrigger() below.
    static void onInput( const InputState& input );  // straightforward feeding to whatever holds context
    static void onTimer( const Timeout& timeout );  
    static void onCollision( const Collision& collision );  // TODO

    auto getTimer() -> Timer*;

  private:
    Trigger() = default;
    Trigger(const Trigger&) = delete;
    Trigger operator=(const Trigger&) = delete;
    Trigger(const Trigger&&) = delete;
    Trigger operator=(const Trigger&&) = delete;

    Timer* _timer;

    struct Activity {
      Priority priority{};
      TimerId animTimerId{};
      TimerId cbTimerId{};
    };

    std::array<Activity, NUM_SUPPORTED_ENTITIES>    _activities{};
		lua_State* luaState{};

};  // class Trigger
