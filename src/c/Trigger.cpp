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
      return -1;
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

	return 0;
}

// int World

void Trigger::init() {
  _timer = &Timer::getInstance();

	// Expose Actions.
  _bridge.addTable<int>( "Action", {
    { "START", Action::START },
    { "STOP", Action::STOP },
    { "PAUSE", Action::PAUSE },
    { "UNPAUSE", Action::UNPAUSE } }
  );

  _bridge.addFunction( "dooody", &doAction );

  // Expose Systems.
  _bridge.addTable<int>( "System", {
    { "TIMER", System::TIMER } }
  );
}

void Trigger::onInput( const InputState& input ) {
	const auto& wm = WindowManager::getInstance();
	const auto currWindow = wm.back();
	assert( currWindow != nullptr );
	currWindow->onInput( input );
  // TODO may be better design to have window return an entityj
}

void Trigger::onTimer( const Timeout& timeout ) {
	// TODO make Lua bridge
}

void Trigger::onCollision( const Collision& collision ) {
  // TODO make Lua bridge
}

auto Trigger::getTimer() -> Timer* {
  return _timer;
}
