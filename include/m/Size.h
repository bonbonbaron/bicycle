#pragma once
#include <lua.hpp>

struct Size {
  unsigned h{};
  unsigned w{};

  void push( lua_State* L ) {
    lua_newtable( L );
    lua_pushinteger( L, w );
    lua_setfield( L, -2, "w" );
    lua_pushinteger( L, h );
    lua_setfield( L, -2, "h" );  // -2 is the table reference's index in the stack
  }
};
