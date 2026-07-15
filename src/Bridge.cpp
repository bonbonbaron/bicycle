#include <lua.hpp>
#include "Bridge.h"
#include "m/World.h"
#include "bicycle.h"

// These functions provide the bridge between Lua scripts and Bicycle's back-end.

Bridge::Bridge() : _luaState( luaL_newstate() ) {
	// TODO expose bicycle's window-management functions
  // luaL_openlibs( _luaState );
  luaopen_base( _luaState );  // lets us do common stuff like pairs()
	
  // TODO expose World items

	if (luaL_dofile(_luaState, "./i.lua") != LUA_OK) {
    auto errStr = std::string( lua_tostring(_luaState, -1) );
		lua_pop(_luaState, 1);
    bicycle::die( errStr );
	}
}

void Bridge::addFunction( const char* name, lua_CFunction f ) {
  lua_register( _luaState, name, f );
}
