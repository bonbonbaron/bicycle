#pragma once
#include "m/Entity.h"
#include "m/Rect.h"
#include "c/CollisionDetector.h"
#include <lua.hpp>

struct Collision {
  Entity lhs{};  // think of this is "self"
  Entity rhs{};  // whom you collided with
  unsigned type{}; // their collision type
};

struct CollRect : public Rect {
  CollRect() = default;
  CollRect( Rect rect, unsigned type ) : Rect(rect), type(type) {}  // register with CollisionDetector
  unsigned type{}; // their collision type
  void push( lua_State* L ) {
    // Position
    lua_newtable(L);
    Rect::push(L);
    lua_pushinteger( L, type );
    lua_setfield( L, -2, "type" );
  }
};

