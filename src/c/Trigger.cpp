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

int Trigger::sys( ) {
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

// int Trigger::getComponent( lua_State* L ) {
extern "C" {
  int Trigger::getComponent( Entity entiy ) {
    return World::get<Rect>( entity );
  }
}

void Trigger::init( const std::string& gameName ) {
  _timer = &Timer::getInstance();

  // TODO move this elswehre and open the base bicycle.lua file here instead.
  std::string gamePath{"./" + gameName + ".lua"};
  if (luaL_dofile( _bridge.getState(), gamePath.c_str() ) != LUA_OK) {
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
