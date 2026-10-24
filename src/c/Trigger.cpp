#include "data/Mem.h"
#include <cstdio>
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
    case System::ANIMATION:
      ctrl = &Animator::getInstance();
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

// require() for the game's own modules (e.g. require("./test/genome/hero")): reads them through assets::read
// (the pack in release builds, the loose file in dev) into the TEXT pool.
// Modules that aren't in the pack fall through to Lua's usual file searchers.
static int packSearcher( lua_State* L ) {
  std::string name = luaL_checkstring( L, 1 );
  std::string path = name;
  if ( path.rfind( "./", 0 ) != 0 ) {
    for ( auto& c : path ) if ( c == '.' ) c = '/';
  }
  path += ".lua";
  auto code = assets::read( path, mem::TEXT );  // the pack (release) or the loose file (dev), into TEXT
#ifdef BICYCLE_RELEASE
  // Engine modules (require("Genome")) are packed as scripts/Genome.lua; dev finds them via package.path.
  if ( !code ) {
    path = "scripts/" + path;
    code = assets::read( path, mem::TEXT );
  }
#endif
  if ( !code ) {
    lua_pushstring( L, ( "\n\tno '" + path + "' in the asset pack or folder" ).c_str() );
    return 1;
  }
  if ( luaL_loadbuffer( L, code->data(), code->size(), ( "@" + path ).c_str() ) != LUA_OK ) {
    return lua_error( L );
  }
  return 1;
}

#ifdef BICYCLE_RELEASE
// Release: Lua can't touch files either. io.open/io.lines/io.popen/io.input/io.output with a file name, dofile,
// loadfile, os.remove/rename/tmpname/execute and package.loadlib answer nil + an error message (so optional
// things like BICYCLE_DEBUG_LOG quietly switch off), and require only knows preloaded and embedded modules.
static int noFileIo( lua_State* L ) {
  lua_pushnil( L );
  lua_pushstring( L, "bicycle release build: no file I/O (everything is embedded)" );
  return 2;
}
static int noFileIoError( lua_State* L ) {
  return luaL_error( L, "bicycle release build: no file I/O (everything is embedded)" );
}
static void sandboxFileIo( lua_State* L ) {
  struct Stub { const char* lib; const char* fn; lua_CFunction f; };
  for ( const Stub& s : { Stub{ "io", "open", noFileIo }, Stub{ "io", "popen", noFileIo }, Stub{ "io", "lines", noFileIoError },
                          Stub{ "io", "input", noFileIoError }, Stub{ "io", "output", noFileIoError },
                          Stub{ "os", "remove", noFileIo }, Stub{ "os", "rename", noFileIo }, Stub{ "os", "tmpname", noFileIoError },
                          Stub{ "os", "execute", noFileIo }, Stub{ "package", "loadlib", noFileIo },
                          Stub{ nullptr, "dofile", noFileIoError }, Stub{ nullptr, "loadfile", noFileIo } } ) {
    if ( s.lib ) {
      lua_getglobal( L, s.lib );
      lua_pushcfunction( L, s.f );
      lua_setfield( L, -2, s.fn );
      lua_pop( L, 1 );
    }
    else {
      lua_pushcfunction( L, s.f );
      lua_setglobal( L, s.fn );
    }
  }
  // require: preload, then the embedded pack; Lua's and C's file searchers are dropped.
  lua_getglobal( L, "package" );
  lua_getfield( L, -1, "loaders" );
  const int n = static_cast<int>( lua_objlen( L, -1 ) );
  for ( int i = n; i >= 3; --i ) {
    lua_pushnil( L );
    lua_rawseti( L, -2, i );
  }
  lua_pop( L, 1 );
  lua_pushstring( L, "" );
  lua_setfield( L, -2, "path" );
  lua_pushstring( L, "" );
  lua_setfield( L, -2, "cpath" );
  lua_pop( L, 1 );
}
#endif

// The Lua heap lives in the TEXT pool. LuaJIT accepts a custom allocator only in GC64 builds (the default on
// x64/arm64 since 2.1); a non-GC64 64-bit LuaJIT returns NULL from lua_newstate, so we fall back to its own
// allocator (mmap'd low memory) and say so.
static void* luaPoolAlloc( void*, void* ptr, std::size_t, std::size_t nsize ) {
  if ( nsize == 0 ) {
    if ( ptr ) mem::memDel( ptr );
    return nullptr;
  }
  return mem::memResize( ptr, nsize, mem::TEXT );
}

void Trigger::init( const std::string& gameName ) {
  L = lua_newstate( luaPoolAlloc, nullptr );
  if ( !L ) {
    std::fprintf( stderr, "bicycle: LuaJIT refused a custom allocator (non-GC64 build); Lua heap is outside the TEXT pool\n" );
    L = luaL_newstate();
  }
  luaL_openlibs(L);
  // The asset pack is mounted in bicycle::init, before the renderer (which loads fonts from it).
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
#ifdef BICYCLE_RELEASE
  sandboxFileIo( L );
#endif
  _timer = &Timer::getInstance();
  _motion = &Kinematics::getInstance();

  // Start the game
  std::string gamePath{"./" + gameName + "/main.lua"};
  auto mainLua = assets::read( gamePath, mem::TEXT );
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
  if ( input.pressed ) {  // dialogues and menus page and select on presses, not on releases too
    const auto& wm = WindowManager::getInstance();
    auto currWindow = wm.back();
    assert( currWindow != nullptr );
    currWindow->onInput( input );
  }
  bridge.input = input;
  bridge.keys.add( input.lastPressed, input.pressed );
}

// TODO determine whether this is talking about mouse movements
void Trigger::sendLook( float dx, float dy ) {
  bridge.lookDx += dx;
  bridge.lookDy += dy;
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

void Trigger::forgetEntity( Entity entity ) {
  if ( entity == 0 ) return;
  for ( unsigned i = 0; i < bridge.collisions.len; ++i ) {
    auto& c = bridge.collisions.arr[i];
    if ( c.lhs == entity || c.rhs == entity ) c.lhs = 0;
  }
  for ( unsigned i = 0; i < bridge.uncollisions.len; ++i ) {
    auto& c = bridge.uncollisions.arr[i];
    if ( c.lhs == entity || c.rhs == entity ) c.lhs = 0;
  }
  for ( unsigned i = 0; i < bridge.timeouts.len; ++i ) {
    if ( bridge.timeouts.arr[i].entity == entity ) bridge.timeouts.arr[i].entity = 0;
  }
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
