#pragma once
#include "c/Input.h"
#include "c/Timeout.h"
#include "c/Collision.h"
#include "c/WindowManager.h"
#include "m/Position.h"
#include "c/Timer.h"
#include "c/Kinematics.h"
#include "m/World.h"
#include <lua.hpp>

 /* "I am the way... Nobody comes to the Father except through me." -John 14:6 
  *
  * Trigger is the gateway through which every signal must pass in the game engine.
  * This is designed to correlate tasks with each other.
  */

// This is needed for older Lua versions.
#ifndef LUA_OK
#define LUA_OK (0)
#endif

class Trigger {
  public:

    // Generic system actions
    enum Action { START, STOP, PAUSE, UNPAUSE };
    enum Component { RECT, COLLRECT, IMAGE };
    enum System{ TIMER, MOTION };

    static auto getInstance() -> Trigger&;
		void init( const std::string& gameName );

    // Le trifecta
    static void sendInput( Input& input );  // straightforward feeding to whatever holds context
    static void sendTimeout( const Timeout& timeout );  
    static void sendCollision( const Collision& collision );
    static void sendUncollision( const Collision& uncollision );
    // An entity going away (despawned, or a 3D body removed mid-dispatch from inside an onCollision): every queued
    // collision/uncollision to or from it and every queued timeout to it is voided in place (lhs/entity = 0, which
    // getUpdates in scripts/bicycle.lua skips), so nothing reaches a dead entity or a later owner of its id, even
    // while Lua is iterating the queue.
    static void forgetEntity( Entity entity );

    void send();  // sends all the above

    auto getTimer() -> Timer*;

    void sys(const Action action, const System system, Entity entity);

  private:
    Trigger();
    Trigger(const Trigger&) = delete;
    Trigger operator=(const Trigger&) = delete;
    Trigger(const Trigger&&) = delete;
    Trigger operator=(const Trigger&&) = delete;

    // Systems
    Timer* _timer;
    Kinematics* _motion;
    
    // Trigger outputs
    static constexpr unsigned MAX_COLLS_PER_ENTITY{10};  // this is arbitrary; surely there's a better data structure for this that can be safely shared with Lua.
    std::vector<Timeout> _timeouts{ MAX_NUM_TIMERS };
    std::vector<Collision> _collisions{ MAX_COLLS_PER_ENTITY * NUM_SUPPORTED_ENTITIES };
    std::vector<Collision> _uncollisions{ MAX_COLLS_PER_ENTITY * NUM_SUPPORTED_ENTITIES };

    // Orchestra (TODO)
    struct Activity {
      Priority priority{};
      TimerId animTimerId{};
      TimerId cbTimerId{};
    };
    std::array<Activity, NUM_SUPPORTED_ENTITIES>    _activities{};
    lua_State *L{};
};  // class Trigger

template<typename T>
struct BridgedArray {
  BridgedArray() {}
  T* arr;
  unsigned len;  // current number of elements 
  unsigned cap;

  // not exported
  void set( std::vector<T>& src ) {
    arr = src.data();
    len = 0;
    cap = src.size();
  }

  // not exported
  void add( const T& val ) {
    if ( len < cap ) {
      arr[ len++ ] = val;
    }
  }
};

// Every key pressed since Lua's last getUpdates(), in order, so no press is lost or merged.
struct KeyQueue {
  static constexpr unsigned CAP{32};
  uint8_t arr[CAP];
  unsigned len;
  void add( const LogicalKey key ) {
    if ( len < CAP ) {
      arr[ len++ ] = static_cast<uint8_t>( key );
    }
  }
};

struct Bridge {
  BridgedArray<Timeout> timeouts;
  BridgedArray<Collision> collisions;
  BridgedArray<Collision> uncollisions;
  KeyQueue keys;  // must precede input: Lua only declares input's first fields
  Input input; // Input is just a scalar.  // TODO should this be initialized?
};
