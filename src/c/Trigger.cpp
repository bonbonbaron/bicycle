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
