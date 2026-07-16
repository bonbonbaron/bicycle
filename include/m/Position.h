#pragma once
#include <lua.hpp>

struct Position {
  Position() = default;
  Position( int x, int y, int z = 0) : x(x), y(y), z(z) {}
  Position( const Position& ) = default;
  Position( Position&& ) = default;
  Position& operator=( const Position& ) = default;
  Position& operator=( Position&& ) = default;
  int x{};
  int y{};
  int z{};

  void operator+=( const Position& rhs ) {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
  }

  auto operator+( const Position& rhs ) const -> Position {
    Position pos;
    pos.x = x + rhs.x;
    pos.y = y + rhs.y;
    pos.z = z + rhs.z;
    return pos;
  }

  void operator-=( const Position& rhs ) {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
  }

  auto operator-( const Position& rhs ) const -> Position {
    Position pos;
    pos.x = x - rhs.x;
    pos.y = y - rhs.y;
    pos.z = z - rhs.z;
    return pos;
  }

  void push( lua_State* L ) {
    lua_newtable(L);
    lua_pushinteger( L, x );
    lua_setfield( L, -2, "x" );
    lua_pushinteger( L, y );
    lua_setfield( L, -2, "y" );
    lua_pushinteger( L, z );
    lua_setfield( L, -2, "z" );
  }
};

