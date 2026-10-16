#pragma once
#include "m/Entity.h"
#include "m/Box.h"

using Velocity = Box;
using Acceleration = Box;

// LINEAR: constant velocity, or along waypoints at a speed (looping or ping-pong).
// ORBITAL: around a point or another entity, on a circle or ellipse.
// COMBO: an orbit whose center travels along waypoints (or orbits a moving entity: that's ORBITAL with a center
//        entity, since the center follows it).
// FOLLOW_LERP: eases toward another entity (plus an offset) each frame, framerate-independently, with a bob.
// See Kinematics::linear/orbit/combo/follow and include/c/MotionMath.h.
enum MotionType { LINEAR, ORBITAL, COMBO, FOLLOW_LERP };

// Advantageous to use an AoS instead of SoA for vectorized additions.

// Useful for restarting/unpausing motion or determining how to handle its final position calculation
struct MotionConfig {
  Velocity initVel{};
  Velocity termVel{};
  Acceleration initAcc{};
  MotionType type{};
  Entity tgt{};  // orbiting, tracking (e.g. missiles), or just general destination
};
