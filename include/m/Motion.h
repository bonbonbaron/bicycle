#pragma once
#include "m/Entity.h"
#include "m/Position.h"
#include "m/Size.h"

using Velocity = Position;
using Acceleration = Position;
using Growth = Size;

enum MotionType { LINEAR, ORBITAL };

// Maybe type will determine the final step of adding velocity to position. 
struct Motion {
  Velocity vel{};
  Acceleration acc{};
  Growth scaleVel{};
  Growth scaleAcc{};
  MotionType type{};
  Entity tgt{};  // orbiting, tracking (e.g. missiles), or just general destination
};
