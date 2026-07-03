#pragma once
#include "m/Entity.h"
#include "m/Rect.h"
#include "c/CollisionDetector.h"

struct Collision {
  Entity lhs{};  // think of this is "self"
  Entity rhs{};  // whom you collided with
  unsigned type{}; // their collision type
};

struct CollRect : public Rect {
  CollRect() = default;
  CollRect( Rect rect, unsigned type ) : Rect(rect), type(type) {}  // register with CollisionDetector
  unsigned type{}; // their collision type
};

