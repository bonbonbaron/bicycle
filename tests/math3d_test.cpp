// True-3D layer: include/m/Math3D.h and src/c/Physics3D.cpp (level collider, character capsule, raycasts,
// overlaps). `ctest -R math3d` or build/bicycle-3d-tests.
#include "m/Math3D.h"
#include "c/Physics3D.h"
#include <cstdio>
#include <vector>

using namespace m3;
static int failures = 0;
#define CHECK( cond, ... ) do { if ( !( cond ) ) { ++failures; std::printf( "FAIL %s:%d: ", __FILE__, __LINE__ ); std::printf( __VA_ARGS__ ); std::printf( "\n" ); } } while ( 0 )
static bool near( float a, float b, float eps = 1e-3f ) { return std::fabs( a - b ) <= eps; }
static bool nearV( Vec3 a, Vec3 b, float eps = 1e-3f ) { return near( a.x, b.x, eps ) && near( a.y, b.y, eps ) && near( a.z, b.z, eps ); }

// A level made of axis-aligned boxes (12 triangles each, outward faces).
struct Level {
  std::vector<float> pos;
  std::vector<uint32_t> tris;
  void box( Vec3 lo, Vec3 hi ) {
    const uint32_t b = static_cast<uint32_t>( pos.size() / 3 );
    for ( int i = 0; i < 8; ++i ) {
      pos.push_back( i & 1 ? hi.x : lo.x );
      pos.push_back( i & 2 ? hi.y : lo.y );
      pos.push_back( i & 4 ? hi.z : lo.z );
    }
    const uint32_t f[12][3] = { { 0, 4, 6 }, { 0, 6, 2 }, { 1, 3, 7 }, { 1, 7, 5 }, { 0, 1, 5 }, { 0, 5, 4 },
                                { 2, 6, 7 }, { 2, 7, 3 }, { 0, 2, 3 }, { 0, 3, 1 }, { 4, 5, 7 }, { 4, 7, 6 } };
    for ( auto& t : f ) for ( uint32_t k : t ) tris.push_back( b + k );
  }
  // A ramp rising along +x from (x0, 0) to (x1, h), z in [z0, z1] (top surface only).
  void ramp( float x0, float x1, float h, float z0, float z1 ) {
    const uint32_t b = static_cast<uint32_t>( pos.size() / 3 );
    for ( float v : { x0, 0.f, z0, x1, h, z0, x1, h, z1, x0, 0.f, z1 } ) pos.push_back( v );
    for ( uint32_t k : { 0u, 2u, 1u, 0u, 3u, 2u } ) tris.push_back( b + k );
  }
};

int main() {
  // --- math --------------------------------------------------------------------------------------------------------
  CHECK( nearV( forward( 0, 0 ), { 0, 0, -1 } ), "yaw 0 looks down -Z" );
  CHECK( nearV( forward( PI / 2, 0 ), { -1, 0, 0 } ), "yaw +90 looks down -X" );
  CHECK( nearV( rotate( fromYawPitch( 0.7f, 0.3f ), { 0, 0, -1 } ), forward( 0.7f, 0.3f ) ), "quat matches forward()" );
  CHECK( nearV( flatRight( 0 ), { 1, 0, 0 } ) && near( dot( flatRight( 1.1f ), flatForward( 1.1f ) ), 0 ), "right basis" );
  CHECK( near( wrapAngle( 3 * PI ), PI ) || near( wrapAngle( 3 * PI ), -PI ), "wrap" );
  {
    const Mat4 m = trs( { 1, 2, 3 }, axisAngle( { 0, 1, 0 }, PI / 2 ), { 2, 2, 2 } );
    CHECK( nearV( transformPoint( m, { 1, 0, 0 } ), { 1, 2, 1 } ), "trs" );
    const Mat4 v = lookAt( { 0, 0, 5 }, { 0, 0, 0 } );
    CHECK( nearV( transformPoint( v, { 0, 0, 0 } ), { 0, 0, -5 } ), "lookAt puts the target in front (-Z)" );
    const Planes p = frustumPlanes( mul( perspective( 1.0f, 1.5f, 0.1f, 100 ), v ) );
    CHECK( sphereVisible( p, { 0, 0, 0 }, 0.5f ), "in front is visible" );
    CHECK( !sphereVisible( p, { 0, 0, 10 }, 0.5f ), "behind is culled" );
    CHECK( !sphereVisible( p, { 50, 0, 0 }, 0.5f ), "far right is culled" );
    CHECK( !sphereVisible( p, { 0, 0, -200 }, 0.5f ), "past far is culled" );
  }
  CHECK( near( rayTriangle( { 0.2f, 1, 0.2f }, { 0, -1, 0 }, { 0, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 } ), 1 ), "ray hits tri" );
  CHECK( rayTriangle( { 2, 1, 2 }, { 0, -1, 0 }, { 0, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 } ) < 0, "ray misses tri" );
  CHECK( near( raySphere( { 0, 0, 0 }, { 1, 0, 0 }, { 5, 0, 0 }, 1 ), 4 ), "ray sphere" );
  CHECK( nearV( closestOnTriangle( { 0.25f, 3, 0.25f }, { 0, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 } ), { 0.25f, 0, 0.25f } ), "closest inside" );
  CHECK( nearV( closestOnTriangle( { -1, 0, -1 }, { 0, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 } ), { 0, 0, 0 } ), "closest vertex" );
  {
    Vec3 c1, c2;
    CHECK( near( segmentSegment( { 0, 0, 0 }, { 0, 2, 0 }, { 1, 1, -1 }, { 1, 1, 1 }, c1, c2 ), 1 ), "segment distance" );
  }
  {
    auto s = spatialize( { 0, 0, 0 }, 0, { 5, 0, 0 }, 2, 50 );
    CHECK( s.pan > 0.99f && s.gain < 1 && s.gain > 0.3f, "right of the listener pans right, attenuated (%f %f)", s.pan, s.gain );
    s = spatialize( { 0, 0, 0 }, PI, { 5, 0, 0 }, 2, 50 );  // turned around: now on the left
    CHECK( s.pan < -0.99f, "turning flips pan" );
    CHECK( spatialize( { 0, 0, 0 }, 0, { 0, 0, -1 }, 2, 50 ).gain == 1, "inside ref: full gain" );
    CHECK( spatialize( { 0, 0, 0 }, 0, { 0, 0, -80 }, 2, 50 ).gain == 0, "past max: silent" );
  }

  // --- physics -----------------------------------------------------------------------------------------------------
  auto& w = phys3d::World3D::get();
  Level lv;
  lv.box( { -20, -1, -20 }, { 20, 0, 20 } );   // floor, top at y = 0
  lv.box( { 5, 0, -20 }, { 6, 3, 20 } );       // wall at x = 5
  lv.box( { -3, 0, -6 }, { -2, 0.25f, -4 } );  // a 0.25 step at x -3..-2
  lv.box( { -8, 0, -6 }, { -7, 1.2f, -4 } );   // a 1.2 block: too tall to step
  lv.ramp( -2, 2, 1.0f, 6, 8 );                // ~14 degree ramp
  w.addLevelTriangles( lv.pos.data(), lv.tris.data(), lv.tris.size() / 3 );
  w.buildGrid( 2 );
  CHECK( w.levelTriangles() == 4 * 12 + 2, "triangles in the level (%zu)", w.levelTriangles() );
  const float DT = 1.f / 30;
  const Entity hero = 1;
  auto& t = w.transform( hero );
  auto& b = w.body( hero );
  t.on = true;
  b.kind = phys3d::CHARACTER;
  t.pos = { 0, 2, 0 };
  for ( int i = 0; i < 40; ++i ) w.step( DT );
  CHECK( b.grounded && near( t.pos.y, 0, 0.02f ), "falls and lands on the floor (y %f, grounded %d)", t.pos.y, b.grounded );
  // Walk into the wall: stops at x = 5 - r.
  b.wish = { 6, 0, 0 };
  for ( int i = 0; i < 60; ++i ) w.step( DT );
  {
    int airborne = 0;  // pushing against a wall must not flicker grounded
    for ( int i = 0; i < 30; ++i ) { w.step( DT ); airborne += !b.grounded; }
    CHECK( airborne == 0, "stays grounded against the wall (%d frames airborne)", airborne );
  }
  CHECK( near( t.pos.x, 5 - b.radius, 0.03f ) && b.grounded, "blocked by the wall (x %f)", t.pos.x );
  // Slide along the wall when walking diagonally into it.
  const float z0 = t.pos.z;
  b.wish = { 4, 0, 4 };
  for ( int i = 0; i < 15; ++i ) w.step( DT );
  CHECK( t.pos.z > z0 + 1 && t.pos.x < 5 - b.radius + 0.03f, "slides along the wall (z %f -> %f, x %f)", z0, t.pos.z, t.pos.x );
  // Jump.
  b.wish = {};
  for ( int i = 0; i < 10; ++i ) w.step( DT );
  b.jumpReq = true;
  w.step( DT );
  float top = t.pos.y;
  for ( int i = 0; i < 40; ++i ) { w.step( DT ); top = std::max( top, t.pos.y ); }
  CHECK( top > 1.2f && b.grounded && near( t.pos.y, 0, 0.02f ), "jumps (apex %f) and lands", top );
  // Steps: walks up the 0.25 step, not up the 1.2 block.
  t.pos = { -1, 0, -5 };
  b.vel = {};
  for ( int i = 0; i < 5; ++i ) w.step( DT );
  b.wish = { -3, 0, 0 };
  for ( int i = 0; i < 20; ++i ) w.step( DT );
  CHECK( t.pos.x < -2.4f && near( t.pos.y, 0.25f, 0.05f ), "climbs the step (x %f y %f)", t.pos.x, t.pos.y );
  for ( int i = 0; i < 60; ++i ) w.step( DT );
  CHECK( t.pos.x > -7 && t.pos.y < 0.3f, "doesn't climb the tall block (x %f y %f)", t.pos.x, t.pos.y );
  // Ramp: walk up to the top.
  t.pos = { -3, 0, 7 };
  b.vel = {};
  b.wish = { 3, 0, 0 };
  for ( int i = 0; i < 50; ++i ) w.step( DT );
  CHECK( t.pos.x > 1 && t.pos.y > 0.7f, "walks up the ramp (x %f y %f)", t.pos.x, t.pos.y );
  // Stand still on the ramp: no sliding down.
  b.wish = {};
  t.pos = { 0, 0.6f, 7 };
  b.vel = {};
  for ( int i = 0; i < 10; ++i ) w.step( DT );
  const Vec3 rest = t.pos;
  for ( int i = 0; i < 30; ++i ) w.step( DT );
  CHECK( nearV( t.pos, rest, 0.03f ) && b.grounded, "stands on the slope (%f %f -> %f %f)", rest.x, rest.y, t.pos.x, t.pos.y );

  // Raycasts: level and entities.
  {
    auto h = w.raycast( { 0, 1, 0 }, { 1, 0, 0 }, 50, 0, false );
    CHECK( h.hit && near( h.t, 5 ) && nearV( h.normal, { -1, 0, 0 } ) && h.entity == 0, "hitscan hits the wall (%f)", h.t );
    h = w.raycast( { 0, 5, 0 }, { 0, -1, 0 }, 50, 0, false );
    CHECK( h.hit && near( h.t, 5 ) && near( h.normal.y, 1 ), "down to the floor" );
    h = w.raycast( { 0, 1, 0 }, { -1, 0, 0 }, 3, 0, false );
    CHECK( !h.hit, "max distance respected" );
    h = w.raycast( { -15, 1, 15 }, { 1, 0, -1 }, 100, 0, false );
    CHECK( h.hit, "diagonal ray through many cells hits the wall" );
    const Entity target = 2;
    w.transform( target ).on = true;
    w.transform( target ).pos = { 2, 0, 0 };
    w.body( target ).kind = phys3d::KINEMATIC;
    w.body( target ).radius = 0.5f;
    w.body( target ).height = 1.8f;
    h = w.raycast( { 0, 1, 0 }, { 1, 0, 0 }, 50, hero, true );
    CHECK( h.hit && h.entity == target && near( h.t, 1.5f, 0.02f ), "hitscan hits the target first (%u at %f)", h.entity, h.t );
    w.transform( target ).on = false;
  }
  // Overlaps report begin/end.
  {
    static int begins = 0, ends = 0;
    w.setContactCallback( []( Entity, Entity, unsigned, bool begin ) { ( begin ? begins : ends )++; } );
    const Entity coin = 3;
    w.transform( coin ).on = true;
    w.body( coin ).kind = phys3d::KINEMATIC;
    w.body( coin ).radius = 0.4f;
    w.body( coin ).height = 0.8f;
    w.body( coin ).collType = 7;
    t.pos = { 0, 0, 0 };
    b.wish = {};
    b.vel = {};
    w.transform( coin ).pos = { 0.5f, 0.3f, 0 };
    w.step( DT );
    CHECK( begins == 1, "overlap begins (%d)", begins );
    w.step( DT );
    CHECK( begins == 1, "reported once" );
    w.transform( coin ).pos = { 5, 0.3f, 0 };
    w.step( DT );
    CHECK( ends == 1, "overlap ends" );
  }
  // Moving platform (BOX) carries a character standing on it.
  {
    const Entity plat = 4;
    w.transform( plat ).on = true;
    w.transform( plat ).pos = { -10, 1, 10 };
    w.body( plat ).kind = phys3d::BOX;
    w.body( plat ).half = { 1.5f, 0.25f, 1.5f };
    t.pos = { -10, 2, 10 };
    b.vel = {};
    for ( int i = 0; i < 30; ++i ) w.step( DT );
    CHECK( b.grounded && near( t.pos.y, 1.25f, 0.03f ), "stands on the platform (y %f)", t.pos.y );
    for ( int i = 0; i < 30; ++i ) { w.transform( plat ).pos.x += 0.05f; w.step( DT ); }
    CHECK( near( t.pos.x, -8.5f, 0.1f ), "carried by it (x %f)", t.pos.x );
  }
  std::printf( failures ? "math3d: %d FAILED\n" : "math3d: all passed\n", failures );
  return failures ? 1 : 0;
}
