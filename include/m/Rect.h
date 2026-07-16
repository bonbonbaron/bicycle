#pragma once
#include "m/Position.h"
#include "m/Size.h"
#include <lua.hpp>

struct Rect {
  Rect() = default;
  Rect( const Position& pos, const Size& size ) : pos(pos), size(size) {}
  Rect( const Rect& ) = default;
  Rect( Rect&& ) = default;

  Rect& operator=( const Rect& ) = default;
  Rect& operator=( Rect&& ) = default;

  Position pos{};
  Size size{};

  auto overlaps( const Rect& rhs ) const -> bool;
  auto crop( const Rect& rhs ) const -> Rect;

  void push( lua_State* L ) {
    // Position
    lua_newtable(L);
    pos.push(L);
    lua_setfield( L, -2, "pos" );
    size.push(L);
    lua_setfield( L, -2, "size" );
  }

};

