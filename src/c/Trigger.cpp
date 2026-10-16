#include "c/Trigger.h"
#include "m/Entity.h"
#include "c/Timer.h"
#include "Constants.h"
#include <cassert>
#include <iostream>
#include "bicycle.h"
#include "c/Animator.h"
#include "data/Pack.h"

extern "C" {
  Bridge bridge;
}

Trigger::Trigger() {
  bridge.collisions.set( _collisions );
  bridge.uncollisions.set( _uncollisions );
  bridge.timeouts.set( _timeouts );
}

auto Trigger::getInstance() -> Trigger& {
  static Trigger trigger;
  return trigger;
}

// This function eases exposure of generic system manipulations.
void Trigger::sys(const Action action, const System system, Entity entity) {
  Controllable* ctrl{};
  switch ( system ) {
    case System::TIMER:
      ctrl = _timer;
      break;
    case System::MOTION:
      ctrl = _motion;
      break;
  }

  assert( ctrl != nullptr );

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

// require() for the game's own modules (e.g. require("./test/genome/hero")): loads them from the asset pack.
// Modules that aren't in the pack fall through to Lua's usual file searchers.
static int packSearcher( lua_State* L ) {
  std::string name = luaL_checkstring( L, 1 );
  if ( !assets::mounted() ) {
    lua_pushstring( L, "\n\tnot in the asset pack (none mounted)" );
    return 1;
  }
  std::string path = name;
  if ( path.rfind( "./", 0 ) != 0 ) {
    for ( auto& c : path ) if ( c == '.' ) c = '/';
  }
  path += ".lua";
  auto code = assets::read( path );
  if ( !code || !assets::exists( path ) ) {
    lua_pushstring( L, ( "\n\tno '" + path + "' in the asset pack" ).c_str() );
    return 1;
  }
  if ( luaL_loadbuffer( L, code->data(), code->size(), ( "@" + path ).c_str() ) != LUA_OK ) {
    return lua_error( L );
  }
  return 1;
}

void Trigger::init( const std::string& gameName ) {
  L = luaL_newstate();
  luaL_openlibs(L);
  // Assets: <game>/assets.bpak if it's there (see include/data/Pack.h).
  assets::mount( gameName );
  lua_getglobal( L, "package" );
  lua_getfield( L, -1, "loaders" );  // LuaJIT (Lua 5.1) calls them loaders
  if ( lua_istable( L, -1 ) ) {
    // Insert at position 2: after the preload searcher, before the file searchers.
    const int n = static_cast<int>( lua_objlen( L, -1 ) );
    for ( int i = n; i >= 2; --i ) {
      lua_rawgeti( L, -1, i );
      lua_rawseti( L, -2, i + 1 );
    }
    lua_pushcfunction( L, packSearcher );
    lua_rawseti( L, -2, 2 );
  }
  lua_pop( L, 2 );
  _timer = &Timer::getInstance();
  _motion = &Kinematics::getInstance();

  // Start the game
  std::string gamePath{"./" + gameName + "/main.lua"};
  auto mainLua = assets::read( gamePath );
  if ( !mainLua ) {
    bicycle::die( "Can't find " + gamePath );
  }
  if ( luaL_loadbuffer( L, mainLua->data(), mainLua->size(), ( "@" + gamePath ).c_str() ) != LUA_OK || lua_pcall( L, 0, LUA_MULTRET, 0 ) != LUA_OK ) {
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
  auto currWindow = wm.back();
  assert( currWindow != nullptr );
  currWindow->onInput( input );
  bridge.input = input;
  bridge.keys.add( input.lastPressed );
}

// Notice timeouts are only sent to outputs.
void Trigger::sendTimeout( const Timeout& timeout ) {
  switch( timeout.addr ) {
    case TimeoutAddr::BRIDGE:
      bridge.timeouts.add( timeout );
      break;
    case TimeoutAddr::ANIMATION:
      Animator::getInstance().tick( timeout.entity );
      break;
    case TimeoutAddr::RENDERING:
      break;
    case TimeoutAddr::MOTION:
      break;
  }
}

void Trigger::sendCollision( const Collision& collision ) {
  bridge.collisions.add( collision );
}

void Trigger::sendUncollision( const Collision& uncollision ) {
  bridge.uncollisions.add( uncollision );
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
}

auto Trigger::getTimer() -> Timer* {
	return _timer;
}
