#include <lua.hpp>

#include "Bridge.h"
#include "bicycle.h"

// These functions provide the bridge between Lua scripts and Bicycle's back-end.

Bridge::Bridge() : _luaState( luaL_newstate() ) {
	// TODO expose bicycle's window-management functions
  // luaL_openlibs( _luaState );
  luaopen_base( _luaState );  // lets us do common stuff like pairs()
	
  // TODO expose World items

}

auto Bridge::getState() -> lua_State* {
  return _luaState;
}

void Bridge::_addFunction( const char* name, lua_CFunction f ) {
   lua_register( _luaState, name, f );
}
