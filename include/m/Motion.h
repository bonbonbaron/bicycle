#pragma once
#include "m/Entity.h"
#include "m/Box.h"

using Velocity = Box;
using Acceleration = Box;

enum MotionType { LINEAR, ORBITAL };

// Advantageous to use an AoS instead of SoA for vectorized additions.

// Useful for restarting/unpausing motion or determining how to handle its final position calculation
struct MotionConfig {
  Velocity initVel{};
  Velocity termVel{};
  Acceleration initAcc{};
  MotionType type{};
  Entity tgt{};  // orbiting, tracking (e.g. missiles), or just general destination
};
