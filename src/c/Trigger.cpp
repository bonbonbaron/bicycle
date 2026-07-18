#include "c/Trigger.h"
#include "m/Entity.h"
#include "c/Timer.h"
#include "Constants.h"
#include <cassert>
#include <iostream>
#include "bicycle.h"

extern "C" {
  Bridge bridge;
}

Trigger::Trigger() {
  bridge.collisions.set( _collisions );
  bridge.timeouts.set( _timeouts );
}

auto Trigger::getInstance() -> Trigger& {
  static Trigger trigger;
  return trigger;
}

// This function eases exposure of generic system manipulations.
void Trigger::sys(const Action action, const System system, Entity entity) {
  auto& trig = getInstance();
  auto* timer = trig.getTimer();

  Controllable* ctrl{};
  switch ( system ) {
    case System::TIMER:
      assert(timer != nullptr );
      ctrl = timer;
      break;
  }

  switch ( action ) {
    case Action::START:
      ctrl->start( entity );
      break;
    case Action::STOP:
      ctrl->stop( entity );
      break;
    case Action::PAUSE:
      ctrl->pause( entity );
      break;
    case Action::UNPAUSE:
      ctrl->unpause( entity );
      break;
  }
}

void Trigger::init( const std::string& gameName ) {
  L = luaL_newstate();
  luaL_openlibs(L);
  _timer = &Timer::getInstance();

  // TODO move this elswehre and open the base bicycle.lua file here instead.
  std::string gamePath{"./" + gameName + ".lua"};
  if (luaL_dofile( L, gamePath.c_str() ) != LUA_OK) {
    auto errStr = std::string( lua_tostring(L, -1) );
    lua_pop(L, 1);
    bicycle::die( errStr );
  }

	lua_getglobal(L, "initBridge");
	if (lua_isfunction(L, -1)) {
		lua_pushlightuserdata(L, (void*) &bridge);
		if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
			const char* err = lua_tostring(L, -1);
			lua_pop(L, 1);
      bicycle::die(err);
		}
	} 
  else {
		lua_pop(L, 1); // pop the non-function
	}
}

void Trigger::sendInput( Input& input ) {
  const auto& wm = WindowManager::getInstance();
  const auto currWindow = wm.back();
  assert( currWindow != nullptr );
  currWindow->onInput( input );
  bridge.input = input;
}

void Trigger::sendTimeout( const Timeout& timeout ) {
  bridge.timeouts.add( timeout );
}

void Trigger::sendCollision( const Collision& collision ) {
  bridge.collisions.add( collision );
}

void Trigger::send() {
	lua_getglobal(L, "getUpdates");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			const char* err = lua_tostring(L, -1);
			lua_pop(L, 1);
      bicycle::die(err);
		}
	} 
  else {
		lua_pop(L, 1); // pop the non-function
	}
  // Reset callback ref.
  bridge.input.triggeredCallbackRef = 0;
}

auto Trigger::getTimer() -> Timer* {
	return _timer;
}
