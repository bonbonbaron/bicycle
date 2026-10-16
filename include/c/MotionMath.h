#pragma once
// Motion math for Kinematics (src/c/Kinematics.cpp), kept free of engine state so tests/codec_test.cpp can check it.
// Units: cells (a sprite pixel is 2 cells wide and 1 tall), seconds, degrees. Positions here are floats; the
// engine stores fixed point (FIXEDPT_DEC_BITS) in World boxes and keeps a float shadow per moving entity so slow
// motions don't stall on rounding.
#include <cmath>

namespace motion {

struct Vec2 {
  float x{}, y{};
};
inline auto operator+( Vec2 a, Vec2 b ) -> Vec2 { return { a.x + b.x, a.y + b.y }; }
inline auto operator-( Vec2 a, Vec2 b ) -> Vec2 { return { a.x - b.x, a.y - b.y }; }
inline auto operator*( Vec2 a, float k ) -> Vec2 { return { a.x * k, a.y * k }; }
inline auto length( Vec2 a ) -> float { return std::sqrt( a.x * a.x + a.y * a.y ); }

constexpr float PI = 3.14159265358979f;
// Cells are twice as tall as they are wide, so a round orbit of `rows` rows is 2 * rows columns wide.
constexpr float CELL_ASPECT = 2.f;

// Framerate-independent smoothing: the fraction of the remaining distance to cover this frame for a stiffness k
// (1/s). After t seconds, exp(-k t) of the original gap is left, however t is cut into frames.
inline auto lerpFactor( float k, float dt ) -> float {
  return k <= 0.f ? 1.f : 1.f - std::exp( -k * dt );
}
inline auto lerp( Vec2 from, Vec2 to, float t ) -> Vec2 { return from + ( to - from ) * t; }

// One step of at most `step` cells toward `target`. Sets arrived when it got there.
inline auto moveToward( Vec2 pos, Vec2 target, float step, bool& arrived ) -> Vec2 {
  const Vec2 d = target - pos;
  const float len = length( d );
  arrived = len <= step || len < 1e-6f;
  return arrived ? target : pos + d * ( step / len );
}

// The point at `angleDeg` on an ellipse with radii rx (columns) and ry (rows) around center. 0 degrees is to the
// right, angles grow clockwise on screen (y points down).
inline auto orbitPoint( Vec2 center, float rx, float ry, float angleDeg ) -> Vec2 {
  const float a = angleDeg * PI / 180.f;
  return { center.x + rx * std::cos( a ), center.y + ry * std::sin( a ) };
}
inline auto orbitAngle( float phaseDeg, float degPerSec, float t ) -> float {
  return std::fmod( phaseDeg + degPerSec * t, 360.f );
}

// Next waypoint index along a path of n points: loop (0,1,..,n-1,0,..) or ping-pong (0,1,..,n-1,n-2,..,0,1,..).
// dir is +1/-1 and flips at the ends when ping-ponging.
inline auto nextWaypoint( int i, int n, bool pingPong, int& dir ) -> int {
  if ( n <= 1 ) return 0;
  if ( !pingPong ) return ( i + 1 ) % n;
  if ( i + dir < 0 || i + dir >= n ) dir = -dir;
  return i + dir;
}

// Dead zone: the follower stays put while the goal is within dead.x/dead.y of it on that axis; past that it only
// closes the excess (so the goal ends up on the zone's edge, like a camera's margins). dead 0 = no dead zone.
inline auto deadZone( Vec2 pos, Vec2 goal, Vec2 dead ) -> Vec2 {
  auto axis = []( float p, float g, float dz ) {
    const float d = g - p;
    if ( d > dz ) return g - dz;
    if ( d < -dz ) return g + dz;
    return p;
  };
  return { axis( pos.x, goal.x, dead.x ), axis( pos.y, goal.y, dead.y ) };
}

// A gentle vertical bob: amplitude in rows, hz cycles per second.
inline auto bob( float amp, float hz, float t ) -> float {
  return amp * std::sin( 2.f * PI * hz * t );
}

}  // namespace motion
