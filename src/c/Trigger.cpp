#include "c/Trigger.h"
#include "m/Entity.h"
#include "c/Timer.h"
#include "Constants.h"
#include <cassert>
#include <iostream>
#include "bicycle.h"

auto Trigger::getInstance() -> Trigger& {
  static Trigger trigger;
  return trigger;
}

int Trigger::doAction( lua_State* luaState ) {
  auto& trig = getInstance();
  // Convert Lua-side args to proper bicycle args.
	Trigger::Action action = static_cast<Trigger::Action>( luaL_checkinteger(luaState, 1) );
	Trigger::System system = static_cast<Trigger::System> ( luaL_checkinteger(luaState, 2) );
	Entity entity = static_cast<Entity>( luaL_checkinteger(luaState, 3) );
  auto* timer = trig.getTimer();

  Controllable* ctrl{};
  switch ( system ) {
    case System::TIMER:
      assert(timer != nullptr );
      ctrl = timer;
      break;
    default:
      std::cout << "i haven't set up that system yet.\n";
      return -1;
  }

	switch ( action ) {
		case Action::START:
			std::cout << "I'm starting\n";
      ctrl->start( entity );
			break;
		case Action::STOP:
			std::cout << "I'm stopping\n";
      ctrl->stop( entity );
			break;
		case Action::PAUSE:
			std::cout << "I'm pausing\n";
      ctrl->pause( entity );
			break;
		case Action::UNPAUSE:
			std::cout << "I'm unpausing\n";
      ctrl->unpause( entity );
			break;
	}

	return 0;
}

void Trigger::init() {
  luaState = luaL_newstate();

  _timer = &Timer::getInstance();

	// Expose Actions.
	lua_newtable(luaState);
	lua_pushinteger(luaState, static_cast<int>(Action::START));
	lua_setfield(luaState, -2, "START");
	lua_pushinteger(luaState, static_cast<int>(Action::STOP));
	lua_setfield(luaState, -2, "STOP");
	lua_pushinteger(luaState, static_cast<int>(Action::PAUSE));
	lua_setfield(luaState, -2, "PAUSE");
	lua_pushinteger(luaState, static_cast<int>(Action::UNPAUSE));
	lua_setfield(luaState, -2, "UNPAUSE");
	lua_setglobal(luaState, "Action");
	lua_register(luaState, "dooody", doAction);

  // Expose Systems.
	lua_newtable(luaState);
	lua_pushinteger(luaState, static_cast<int>(System::TIMER));
	lua_setfield(luaState, -2, "TIMER");
	lua_setglobal(luaState, "System");

	// TODO expose bicycle's window-management functions
	

	// TODO Put Trigger functions here.

	if (luaL_dofile(luaState, "./i.lua") != LUA_OK) {
    auto errStr = std::string( lua_tostring(luaState, -1) );
		lua_pop(luaState, 1);
    bicycle::die( errStr );
	}
}

void Trigger::onInput( const InputState& input ) {
	const auto& wm = WindowManager::getInstance();
	const auto currWindow = wm.back();
	assert( currWindow != nullptr );
	currWindow->onInput( input );
}

void Trigger::onTimer( const Timeout& timeout ) {
	// onTrigger( timeout.entity, timeout );  // TODO replace with Lua bridge (try to make common) 
}

void Trigger::onCollision( const Collision& collision ) {
  // onTrigger( collision.lhs, collision );  // TODO replace with Lua bridge (try to make common) 
}

auto Trigger::getTimer() -> Timer* {
  return _timer;
}
