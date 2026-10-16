// Motion math (include/c/MotionMath.h) used by Kinematics: LINEAR paths, ORBITAL/COMBO orbits, FOLLOW_LERP (the
// fairy and the camera). `ctest -R motion` or build/bicycle-motion-tests.
#include "c/MotionMath.h"
#include <cstdio>
#include <initializer_list>
#include <cmath>

using namespace motion;
static int failures = 0;
#define CHECK( cond, ... ) do { if ( !( cond ) ) { ++failures; std::printf( "FAIL %s:%d: ", __FILE__, __LINE__ ); std::printf( __VA_ARGS__ ); std::printf( "\n" ); } } while ( 0 )
static bool near( float a, float b, float eps = 1e-3f ) { return std::fabs( a - b ) <= eps; }

// Follow a fixed goal for `seconds` at `fps`; returns the remaining distance.
static float followFor( float k, float fps, float seconds ) {
  Vec2 p{ 0, 0 };
  const Vec2 goal{ 100, 50 };
  const int frames = static_cast<int>( std::lround( seconds * fps ) );
  for ( int i = 0; i < frames; ++i ) p = lerp( p, goal, lerpFactor( k, 1.f / fps ) );
  return length( goal - p );
}

int main() {
  // FOLLOW_LERP: framerate-independent. After t seconds, exp(-k t) of the gap is left at any frame rate.
  const float gap0 = length( Vec2{ 100, 50 } );
  for ( float fps : { 15.f, 30.f, 60.f, 144.f } ) {
    const float left = followFor( 4.f, fps, 1.f );
    CHECK( near( left, gap0 * std::exp( -4.f ), 1e-2f ), "follow at %g fps left %g, want %g", fps, left, gap0 * std::exp( -4.f ) );
  }
  CHECK( near( followFor( 4.f, 30.f, 0.5f ), followFor( 4.f, 120.f, 0.5f ), 1e-2f ), "30 vs 120 fps differ" );
  // Hard tracking (rate 0) snaps in one frame.
  CHECK( lerpFactor( 0.f, 1.f / 30 ) == 1.f, "rate 0 must snap" );
  CHECK( followFor( 0.f, 30.f, 1.f / 30 ) < 1e-4f, "hard follow didn't arrive" );
  // Never overshoots.
  CHECK( lerpFactor( 1000.f, 1.f ) <= 1.f && lerpFactor( 3.f, 1.f / 30 ) > 0.f, "lerp factor out of (0, 1]" );

  // Dead zone: no movement inside, and only the excess outside.
  Vec2 dz = deadZone( { 10, 10 }, { 13, 9 }, { 5, 2 } );
  CHECK( near( dz.x, 10 ) && near( dz.y, 10 ), "inside the dead zone moved: %g,%g", dz.x, dz.y );
  dz = deadZone( { 10, 10 }, { 18, 4 }, { 5, 2 } );
  CHECK( near( dz.x, 13 ) && near( dz.y, 6 ), "dead zone excess wrong: %g,%g", dz.x, dz.y );

  // LINEAR: steps of `step`, then arrival exactly on the target.
  bool arrived{};
  Vec2 p = moveToward( { 0, 0 }, { 10, 0 }, 4, arrived );
  CHECK( near( p.x, 4 ) && !arrived, "first step" );
  p = moveToward( p, { 10, 0 }, 4, arrived );
  p = moveToward( p, { 10, 0 }, 4, arrived );
  CHECK( near( p.x, 10 ) && arrived, "arrival %g", p.x );
  // Waypoints: loop and ping-pong.
  int dir = 1, i = 0;
  int seq[8];
  for ( int& s : seq ) { s = i; i = nextWaypoint( i, 3, true, dir ); }
  const int pp[8] = { 0, 1, 2, 1, 0, 1, 2, 1 };
  for ( int k = 0; k < 8; ++k ) CHECK( seq[k] == pp[k], "ping-pong step %d: %d, want %d", k, seq[k], pp[k] );
  dir = 1;
  CHECK( nextWaypoint( 2, 3, false, dir ) == 0, "loop wraps" );

  // ORBITAL: on the ellipse at every angle; a "round" orbit is twice as wide in cells (pixels are 2x1 cells).
  const Vec2 c{ 50, 20 };
  for ( float a = 0; a < 360; a += 15 ) {
    const Vec2 o = orbitPoint( c, 2 * 5 * CELL_ASPECT / 2, 5, a );
    const float ex = ( o.x - c.x ) / 10, ey = ( o.y - c.y ) / 5;
    CHECK( near( ex * ex + ey * ey, 1.f ), "off the ellipse at %g", a );
  }
  CHECK( near( orbitPoint( c, 10, 5, 90 ).y, 25 ), "90 degrees is below (screen y down)" );
  CHECK( near( orbitAngle( 30, 90, 2 ), 210 ), "angle after 2 s" );
  CHECK( near( orbitAngle( 0, 90, 5 ), 90 ), "angle wraps at 360" );
  // COMBO: an orbit around a center moving linearly stays radius away from that center.
  Vec2 center{ 0, 0 };
  for ( int f = 0; f < 60; ++f ) {
    center = moveToward( center, { 40, 0 }, 0.5f, arrived );
    const Vec2 o = orbitPoint( center, 6, 3, orbitAngle( 0, 180, f / 30.f ) );
    const float ex = ( o.x - center.x ) / 6, ey = ( o.y - center.y ) / 3;
    CHECK( near( ex * ex + ey * ey, 1.f ), "combo off its moving orbit at frame %d", f );
  }
  // Bob stays within its amplitude.
  for ( float t = 0; t < 3; t += 0.1f ) CHECK( std::fabs( bob( 0.6f, 1.1f, t ) ) <= 0.6f + 1e-5f, "bob too big" );

  std::printf( failures ? "%d FAILURES\n" : "ALL MOTION TESTS PASSED\n", failures );
  return failures ? 1 : 0;
}
