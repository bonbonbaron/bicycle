// ActiveFirst (include/data/ActiveFirst.h) and what's built on it: CollisionDetector's active-first check() and
// step blocking, Kinematics' moving set, Timer's running set. `ctest -R active-first` or build/bicycle-active-first-tests.
// Trigger is stubbed here: its sends just record what the engine would have told Lua.
#include "data/ActiveFirst.h"
#include "c/CollisionDetector.h"
#include "c/Kinematics.h"
#include "c/Timer.h"
#include "c/ImageOverlaps.h"
#include "v/common/FlatRenderer.h"
#include "c/Trigger.h"
#include "m/World.h"
#include <algorithm>
#include <cstdio>
#include <random>
#include <set>
#include <vector>

static int failures = 0;
#define CHECK( cond, ... ) do { if ( !( cond ) ) { ++failures; std::printf( "FAIL %s:%d: ", __FILE__, __LINE__ ); std::printf( __VA_ARGS__ ); std::printf( "\n" ); } } while ( 0 )

// --- Trigger stand-in ------------------------------------------------------------------------------------------------
static std::vector<Collision> colls, uncolls;
static std::vector<Timeout> timeouts;
void Trigger::sendCollision( const Collision& c ) { colls.push_back( c ); }
void Trigger::sendUncollision( const Collision& c ) { uncolls.push_back( c ); }
void Trigger::sendTimeout( const Timeout& t ) { timeouts.push_back( t ); }
static auto heard( const std::vector<Collision>& v, Entity lhs, Entity rhs ) -> bool {
  return std::any_of( v.begin(), v.end(), [&]( const Collision& c ) { return c.lhs == lhs && c.rhs == rhs; } );
}
static void clearEvents() { colls.clear(); uncolls.clear(); timeouts.clear(); }

// --- the container ---------------------------------------------------------------------------------------------------
static void testContainer() {
  ActiveFirst<int, float> t;
  CHECK( t.add( 7, 70, 7.f ) && t.add( 3, 30, 3.f ) && t.add( 9, 90, 9.f ), "add" );
  CHECK( !t.add( 7, 0, 0.f ), "a key twice" );
  CHECK( t.size() == 3 && t.nActive() == 0, "all inactive at first" );
  CHECK( t.activate( 9 ) && !t.activate( 9 ), "activate once" );
  CHECK( t.nActive() == 1 && t.key( 0 ) == 9 && t.col<0>()[0] == 90, "active row first, columns along" );
  t.activate( 3 );
  t.deactivate( 9 );
  CHECK( t.nActive() == 1 && t.key( 0 ) == 3 && t.at<1>( 9 ) == 9.f, "deactivate" );
  t.remove( 3 );
  CHECK( !t.contains( 3 ) && t.nActive() == 0 && t.size() == 2 && t.at<0>( 7 ) == 70, "remove an active row" );

  // Random operations against a plain model: every key keeps its cells, and the active ones are exactly the prefix.
  ActiveFirst<uint32_t> r;
  std::set<uint32_t> in, on;
  std::mt19937 rng( 1 );
  for ( int i = 0; i < 20000; ++i ) {
    const uint32_t k = rng() % 64;
    switch ( rng() % 4 ) {
      case 0: if ( r.add( k, k * 10 ) ) in.insert( k ); break;
      case 1: r.remove( k ); in.erase( k ); on.erase( k ); break;
      case 2: r.activate( k ); if ( in.count( k ) ) on.insert( k ); break;
      case 3: r.deactivate( k ); on.erase( k ); break;
    }
  }
  CHECK( r.size() == in.size() && r.nActive() == on.size(), "sizes %u/%u vs %zu/%zu", r.size(), r.nActive(), in.size(), on.size() );
  for ( uint32_t row = 0; row < r.size(); ++row ) {
    const uint32_t k = r.key( row );
    CHECK( r.row( k ) == row && r.col<0>()[row] == k * 10, "row %u: key %u lost its cell or index", row, k );
    CHECK( ( row < r.nActive() ) == ( on.count( k ) == 1 ), "key %u on the wrong side", k );
  }
}

// --- collisions ------------------------------------------------------------------------------------------------------
constexpr unsigned HERO = 1, SOLID = 2, COIN = 3, ENEMY = 4;
constexpr int CELL = 256;
static void place( Entity e, int x, int y, int w, int h, unsigned type ) {
  World::set<Box>( e, Box( Position( x, y ), Size( w, h ) ) );
  World::set<CollBox>( e, CollBox( Box(), type ) );
  CollisionDetector::registerEntity( e, 0 );
}
static void frame() {  // what bicycle.cpp does between Lua's turns
  Kinematics::getInstance().run();
  CollisionDetector::check();
}

static void testCollisions() {
  auto& cd = CollisionDetector::getInstance();
  auto& kin = Kinematics::getInstance();
  const Entity hero = 10, wall = 11, coin = 12, horse = 13, rockA = 14, rockB = 15;
  place( hero, 10, 10, 2, 2, HERO );
  place( wall, 8, 10, 2, 2, SOLID );     // just left of the hero
  place( coin, 13, 10, 1, 1, COIN );     // one cell right of the hero
  place( horse, 10, 20, 2, 2, ENEMY );
  place( rockA, 40, 40, 2, 2, SOLID );   // two still things overlapping: nobody checks still pairs
  place( rockB, 41, 40, 2, 2, SOLID );
  cd.setBlockers( hero, 1u << SOLID | 1u << ENEMY );
  frame();
  CHECK( colls.empty(), "still entities collided: %zu events", colls.size() );

  // Into the wall (it starts flush with it): the collision is reported, and the hero is pushed back out, flush.
  CHECK( kin.step( hero, Position( -1, 0 ) ), "step refused before touching anything" );
  CHECK( kin.isMoving( hero ) && cd.isActive( hero ), "a stepped entity isn't active" );
  frame();
  CHECK( heard( colls, hero, wall ) && heard( colls, wall, hero ), "hero/wall collision not sent" );
  CHECK( World::get<Box>( hero ).pos.x == 10 * CELL, "not pushed out flush (x=%d)", World::get<Box>( hero ).pos.x );
  CHECK( cd.stepBlocked( hero ), "stepBlocked not set" );
  clearEvents();
  frame();
  CHECK( uncolls.empty() && cd.touching( hero, wall ), "flush against the wall, but not touching it any more" );
  CHECK( !kin.step( hero, Position( -1, 0 ) ), "stepped further into the wall it's flush with" );

  // Onto the coin: not a blocker, so the step stands (and leaving the wall ends that contact).
  clearEvents();
  CHECK( kin.step( hero, Position( 2, 0 ) ), "step to the coin refused" );
  frame();
  CHECK( heard( colls, hero, coin ) && World::get<Box>( hero ).pos.x == 12 * CELL && !cd.stepBlocked( hero ), "coin" );
  CHECK( heard( uncolls, hero, wall ), "stepping away from the wall didn't end the contact" );
  kin.teleport( coin, Position( 100, 100 ) );  // collected: it moves away
  clearEvents();
  frame();
  CHECK( heard( uncolls, coin, hero ), "the teleported coin still touches the hero" );

  // Settled: after two checks without moving, the hero goes still again.
  frame();
  frame();
  CHECK( !cd.isActive( hero ) && !kin.isMoving( hero ), "the hero never settled" );

  // A moving enemy walks into the hero: the hero may not step toward it while touching it, but may step away.
  kin.teleport( horse, Position( 12, 13 ) );
  kin.setVelocity( horse, Position( 0, -1 ) );  // one cell up per frame
  clearEvents();
  frame();
  frame();
  CHECK( heard( colls, hero, horse ), "the horse never reached the hero" );
  kin.setVelocity( horse, Position() );
  CHECK( !kin.step( hero, Position( 0, 1 ) ), "stepped into the enemy it touches" );
  CHECK( World::get<Box>( hero ).pos.y == 10 * CELL, "a refused step moved the hero" );
  CHECK( kin.step( hero, Position( 0, -1 ) ), "stepping away refused" );
  clearEvents();
  frame();
  CHECK( uncolls.empty(), "flush with the horse (it blocks the hero), so still touching" );
  CHECK( kin.step( hero, Position( 0, -1 ) ), "stepping further away refused" );
  frame();
  frame();
  CHECK( heard( uncolls, hero, horse ), "no uncollision after stepping away" );

  // Despawn: whoever touched it hears it go.
  kin.teleport( horse, Position( 12, 9 ) );
  frame();
  clearEvents();
  cd.removeEntity( horse );
  kin.forget( horse );
  CHECK( heard( uncolls, hero, horse ), "removing the horse left a stale contact" );
}

// --- timers ----------------------------------------------------------------------------------------------------------
// Blocking (setBlockers): gaps close flush, only the component into the blocker stops, backing away always works.
static void testBlocking() {
  auto& cd = CollisionDetector::getInstance();
  auto& kin = Kinematics::getInstance();
  const Entity box = 30, wallR = 31, floor = 32, slider = 33;
  auto pos = []( Entity e ) { return World::get<Box>( e ).pos; };
  place( box, 100, 100, 2, 2, HERO );
  place( wallR, 105, 90, 2, 20, SOLID );  // its left edge at x = 105: a 3-cell gap to the box's right edge
  place( floor, 90, 106, 30, 2, SOLID );  // its top at y = 106: 4 cells below the box
  cd.setBlockers( box, 1u << SOLID );
  frame();
  clearEvents();

  // A step bigger than the gap ends flush against the wall, not back where it started.
  CHECK( kin.step( box, Position( 5, 0 ) ), "step refused" );
  frame();
  CHECK( pos( box ).x == 103 * CELL && pos( box ).y == 100 * CELL, "not flush: x=%d (want %d)", pos( box ).x, 103 * CELL );
  CHECK( heard( colls, box, wallR ) && cd.stepBlocked( box ), "the collision or stepBlocked is missing" );
  // Flush: a step into the wall and down slides (only x is dropped); back away while touching is fine.
  CHECK( kin.step( box, Position( 1, 1 ) ), "a diagonal step along the wall refused" );
  frame();
  CHECK( pos( box ).x == 103 * CELL && pos( box ).y == 101 * CELL, "no slide: %d,%d", pos( box ).x, pos( box ).y );
  CHECK( kin.step( box, Position( -1, 0 ) ), "backing away from the wall refused" );
  frame();
  CHECK( pos( box ).x == 102 * CELL && !cd.touching( box, wallR ), "didn't back away" );

  // Velocity: moving right and down into the wall keeps sliding down (only vx stops), then lands flush on the floor.
  kin.teleport( box, Position( 100, 100 ) );
  frame();
  kin.setVelocity( box, Position( 2, 1 ) );
  frame();  // x 102, y 101
  frame();  // x 104 -> pushed to 103, vx stopped
  CHECK( pos( box ).x == 103 * CELL && pos( box ).y == 102 * CELL, "velocity into the wall: %d,%d", pos( box ).x, pos( box ).y );
  for ( int i = 0; i < 4; ++i ) frame();
  CHECK( pos( box ).x == 103 * CELL && pos( box ).y == 104 * CELL, "not resting in the corner: %d,%d", pos( box ).x, pos( box ).y );
  CHECK( cd.touching( box, wallR ) && cd.touching( box, floor ), "the corner's walls don't both touch it" );
  CHECK( !kin.isMoving( box ) || pos( box ).x == 103 * CELL, "still drifting" );
  // Backing out of the corner works at once, both ways.
  kin.setVelocity( box, Position( -1, -1 ) );
  frame();
  CHECK( pos( box ).x == 102 * CELL && pos( box ).y == 103 * CELL, "couldn't back out of the corner" );
  kin.setVelocity( box, Position() );

  // Corner, diagonally from the open side: a step that would cross both edges stops on the one it reaches last.
  place( slider, 120, 100, 2, 2, HERO );
  cd.setBlockers( slider, 1u << SOLID );
  kin.teleport( slider, Position( 121, 103 ) );  // right of the floor's end (x 120), 1 row above its top
  frame();
  CHECK( kin.step( slider, Position( -3, 2 ) ), "corner step refused" );
  frame();  // the floor's right edge x=120 is reached after 1/3 of dx, its top y=106 after 1/2 of dy: lands on top
  CHECK( pos( slider ).y == 104 * CELL && pos( slider ).x == 118 * CELL, "corner: %d,%d", pos( slider ).x, pos( slider ).y );
  for ( Entity e : { box, wallR, floor, slider } ) { cd.removeEntity( e ); kin.forget( e ); }
  clearEvents();
}

// Image overlap groups (ImageOverlaps) and the flat renderers' group-wise depth sort (FlatRenderer.h).
static void testImageOverlaps() {
  auto& io = ImageOverlaps::getInstance();
  auto& kin = Kinematics::getInstance();
  const Entity layer = 40, other = 41, tree = 42, hero = 43, sign = 44, bush = 45, far = 46, ghost = 47;
  auto put = [&]( Entity e, int x, int y, int w, int h, Entity l ) {
    World::set<Box>( e, Box( Position( x, y ), Size( w, h ) ) );
    io.add( e, l );
  };
  put( tree, 10, 0, 4, 10, layer );   // feet at y 10
  put( hero, 12, 4, 2, 4, layer );    // feet at 8: overlaps the tree, so drawn before (behind) it
  put( sign, 30, 0, 2, 2, layer );    // alone
  put( bush, 13, 9, 4, 3, layer );    // feet at 12: overlaps the tree (and so joins its group)
  put( far, 60, 0, 2, 2, layer );
  put( ghost, 12, 4, 2, 4, other );   // same place as the hero, another layer: never grouped with it
  frame();  // CollisionDetector::check updates the groups
  CHECK( io.overlapping( tree, hero ) && io.overlapping( tree, bush ) && !io.overlapping( hero, bush ), "overlaps" );
  CHECK( !io.overlapping( hero, ghost ) && io.group( ghost ) == ImageOverlaps::NONE, "grouped across layers" );
  CHECK( io.group( tree ) != ImageOverlaps::NONE && io.group( tree ) == io.group( hero ) && io.group( hero ) == io.group( bush ),
         "the tree, hero and bush aren't one group" );
  CHECK( io.group( sign ) == ImageOverlaps::NONE && io.grouped( layer ) && !io.grouped( other ), "groups" );

  // Sorted by feet within the group's slots (0, 1, 3); the sign and far keep slots 2 and 4.
  std::vector<Entity> order{ tree, hero, sign, bush, far };
  std::vector<std::pair<Entity, uint32_t>> slots;
  std::vector<Entity> members;
  const auto foot = []( Entity e ) { const Box& b = World::get<Box>( e ); return b.pos.y + static_cast<int>( b.size.h ); };
  const auto groupOf = [&]( Entity e ) { return io.group( e ); };
  gfx::flat::sortGroups( order, groupOf, foot, slots, members );
  CHECK( ( order == std::vector<Entity>{ hero, tree, sign, bush, far } ), "sorted %u %u %u %u %u", order[0], order[1], order[2], order[3], order[4] );

  // The hero walks below the tree (feet past its feet): drawn in front of it. Only the mover is rechecked.
  kin.teleport( hero, Position( 12, 7 ) );  // feet at 11
  frame();
  order = { tree, hero, sign, bush, far };
  gfx::flat::sortGroups( order, groupOf, foot, slots, members );
  CHECK( ( order == std::vector<Entity>{ tree, hero, sign, bush, far } ), "the hero in front" );
  // Walking away breaks the group up: back to the added order.
  kin.teleport( hero, Position( 40, 20 ) );
  frame();
  CHECK( io.group( hero ) == ImageOverlaps::NONE && io.group( tree ) == io.group( bush ), "regrouped" );
  kin.teleport( bush, Position( 80, 20 ) );
  frame();
  CHECK( !io.grouped( layer ), "the layer still has a group" );
  for ( Entity e : { tree, hero, sign, bush, far, ghost } ) { io.remove( e ); kin.forget( e ); }
  frame();
}

static void testTimers() {
  auto& t = Timer::getInstance();
  clearEvents();
  const TimerId once = t.create( 100, 1, 0, false ), rep = t.create( 100, 2, 0, true ), paused = t.create( 100, 3, 0, true );
  t.pause( paused );
  const unsigned period = ( Constants::FRAMES_PER_SECOND * 100 ) / 1000;
  int onceFired = 0, repFired = 0, pausedFired = 0;
  for ( unsigned f = 0; f < period * 3; ++f ) {
    t._run();
    for ( const auto& to : timeouts ) {
      onceFired += to.id == once;
      repFired += to.id == rep && to.entity == 2;
      pausedFired += to.entity == 3;
    }
    timeouts.clear();
  }
  CHECK( onceFired == 1 && repFired == 3 && pausedFired == 0, "fired once %d, repeating %d, paused %d", onceFired, repFired, pausedFired );
  t.unpause( paused );
  for ( unsigned f = 0; f < period; ++f ) t._run();
  CHECK( std::count_if( timeouts.begin(), timeouts.end(), []( const Timeout& to ) { return to.entity == 3; } ) == 1, "unpaused timer didn't fire" );
  t.stopEntity( 2 );
  t.stopEntity( 3 );
  timeouts.clear();
  for ( unsigned f = 0; f < period * 2; ++f ) t._run();
  CHECK( timeouts.empty(), "stopped timers fired" );
}

int main() {
  testContainer();
  testCollisions();
  testBlocking();
  testImageOverlaps();
  testTimers();
  if ( failures == 0 ) std::printf( "active-first: all passed\n" );
  return failures == 0 ? 0 : 1;
}
