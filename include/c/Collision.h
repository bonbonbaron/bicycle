#pragma once
#include "m/Entity.h"
#include "m/Box.h"
#include "c/CollisionDetector.h"

struct Collision {
  Entity lhs{};  // think of this is "self"
  Entity rhs{};  // whom you collided with
  unsigned type{}; // their collision type
};

struct CollBox : public Box {
  CollBox() = default;
  CollBox( Box box, unsigned type ) : Box(box), type(type) {}  // register with CollisionDetector
  unsigned type{}; // their collision type
};

