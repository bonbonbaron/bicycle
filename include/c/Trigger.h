#pragma once
#include <unordered_map>
#include "c/InputData.h"
#include "c/Timeout.h"
#include "c/Collision.h"
#include "c/WindowManager.h"
#include "m/Position.h"
#include "c/Timer.h"
#include "Bridge.h"
#include "m/World.h"
#include <lua.hpp>

 /* "I am the way... Nobody comes to the Father except through me." -John 14:6 
  *
  * Trigger is the gateway through which every signal must pass in the game engine.
  * This is designed to correlate tasks with each other.
  */

#ifndef LUA_OK
#define LUA_OK (0)
#endif

class Trigger {
  public:

    static auto getInstance() -> Trigger&;
		void init( const std::string& gameName );

    // Le trifecta
    static void onInput( const InputState& input );  // straightforward feeding to whatever holds context
    static void onTimer( const Timeout& timeout );  
    static void onCollision( const Collision& collision );  // TODO

    auto getTimer() -> Timer*;

    auto getBridge() -> Bridge&;

  private:
    Trigger() = default;
    Trigger(const Trigger&) = delete;
    Trigger operator=(const Trigger&) = delete;
    Trigger(const Trigger&&) = delete;
    Trigger operator=(const Trigger&&) = delete;

    // Generic system actions
    enum Action { START, STOP, PAUSE, UNPAUSE };
    enum Component { RECT, COLLRECT, IMAGE };
    enum System{ TIMER };

		static int sys( lua_State* luaState );
    static int getComponent( lua_State* luaState );
    // Systems
    Timer* _timer;

    // Orchestra (TODO)
    struct Activity {
      Priority priority{};
      TimerId animTimerId{};
      TimerId cbTimerId{};
    };
    std::array<Activity, NUM_SUPPORTED_ENTITIES>    _activities{};

    Bridge _bridge{};
};  // class Trigger
