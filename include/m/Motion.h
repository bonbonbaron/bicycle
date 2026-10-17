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
// LINEAR3D/ORBITAL3D/FOLLOW3D: the same on a 3D entity's transform (include/c/Physics3D.h), in meters; 2D boxes
// are untouched by them.
// VIEW3D: rides along in another entity's view (its yaw and pitch: e.g. a gun in front of the camera).
enum MotionType { LINEAR, ORBITAL, COMBO, FOLLOW_LERP, LINEAR3D, ORBITAL3D, FOLLOW3D, VIEW3D };

// Advantageous to use an AoS instead of SoA for vectorized additions.

// Useful for restarting/unpausing motion or determining how to handle its final position calculation
struct MotionConfig {
  Velocity initVel{};
  Velocity termVel{};
  Acceleration initAcc{};
  MotionType type{};
  Entity tgt{};  // orbiting, tracking (e.g. missiles), or just general destination
};
