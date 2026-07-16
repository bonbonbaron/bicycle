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

int Trigger::sys( lua_State* L ) {
  auto& trig = getInstance();
  // Convert Lua-side args to proper bicycle args.
	Trigger::Action action = static_cast<Action>( luaL_checkinteger(L, 1) );
	Trigger::System system = static_cast<System> ( luaL_checkinteger(L, 2) );
	Entity entity = static_cast<Entity>( luaL_checkinteger(L, 3) );
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

auto Trigger::getBridge() -> Bridge& {
  return _bridge;
}

int Trigger::getComponent( lua_State* L ) {
  auto& trig = Trigger::getInstance();
  auto& bridge = trig.getBridge();
  Component compType = static_cast<Component>( luaL_checkinteger(L, 1) );
  Entity entity = static_cast<Entity>( luaL_checkinteger(L, 2) );

    if ( compType == Component::RECT ) {
      auto& t = World::get<Rect>( entity );  // Hey LUA, thanks for forcing me to be explicit so that nothing can be elegant.
      t.push( L );
    }
    else if ( compType == Component::COLLRECT ) {
      auto& t = World::get<CollRect>( entity );
      t.push( L );
    }
    else if ( compType == Component::IMAGE ) {
      auto& t = World::get<Image>( entity );
      // t.push( L );
    }
  return 1;
}

void Trigger::init( const std::string& gameName ) {
  _timer = &Timer::getInstance();

	// Expose Actions.
  _bridge.addTable<int>( "Action", {
    { "START", Action::START },
    { "STOP", Action::STOP },
    { "PAUSE", Action::PAUSE },
    { "UNPAUSE", Action::UNPAUSE } }
  );

  addFunction( sys );

  // Expose Systems.
  _bridge.addTable<int>( "System", {
    { "TIMER", System::TIMER } }
  );

	// Expose Components.
  _bridge.addTable<int>( "Component", {
      { "RECT", Component::RECT }, 
      { "COLLRECT", Component::COLLRECT }, 
      { "IMAGE", Component::IMAGE }, }
  );
  addFunction( getComponent );

	if (luaL_dofile(_bridge.getState(), "./i.lua") != LUA_OK) {
    auto errStr = std::string( lua_tostring(_bridge.getState(), -1) );
		lua_pop(_bridge.getState(), 1);
    bicycle::die( errStr );
	}
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
