// The collision check before and after the active-first table: the old one paired every entity on a layer with
// every other each frame (and gathered both boxes per pair); the new one pairs only moving entities with the rest.
// Same world, same motion, n entities of which some move: microseconds per check(). Not a ctest (timing).
//   build/bicycle-collision-bench
#include "c/CollisionDetector.h"
#include "c/Kinematics.h"
#include "c/Trigger.h"
#include "m/World.h"
#include <chrono>
#include <cstdio>
#include <random>
#include <unordered_set>
#include <vector>

static size_t nEvents = 0;
void Trigger::sendCollision( const Collision& ) { ++nEvents; }
void Trigger::sendUncollision( const Collision& ) { ++nEvents; }
void Trigger::sendTimeout( const Timeout& ) {}

namespace old {  // CollisionDetector::check() as it was (c6e951a), for one layer
auto getCollBox( const Entity entity ) -> CollBox {
  const auto& box = World::get<Box>( entity );
  auto coll = World::get<CollBox>( entity );
  if ( coll.size.w == 0 || coll.size.h == 0 ) { coll.size = box.size; coll.pos = Position{}; }
  coll.pos += box.pos;
  return coll;
}
struct PairHash {
  std::size_t operator()( const std::pair<Entity, Entity>& p ) const {
    return std::hash<Entity>{}( p.first ) ^ ( std::hash<Entity>{}( p.second ) << 1 );
  }
};
std::unordered_set<std::pair<Entity, Entity>, PairHash> recorded;
void check( const std::vector<Entity>& layer ) {
  for ( auto e1 = layer.begin(); e1 != layer.end(); ++e1 ) {
    const auto f1 = getCollBox( *e1 );
    for ( auto e2 = std::next( e1 ); e2 != layer.end(); ++e2 ) {
      const bool were = recorded.contains( { *e1, *e2 } );
      const auto f2 = getCollBox( *e2 );
      if ( f1.overlaps( f2 ) ) {
        if ( !were ) { Trigger::sendCollision( { *e1, *e2, f2.type } ); Trigger::sendCollision( { *e2, *e1, f1.type } ); recorded.insert( { *e1, *e2 } ); }
      }
      else if ( were ) { Trigger::sendUncollision( { *e1, *e2, f2.type } ); Trigger::sendUncollision( { *e2, *e1, f1.type } ); recorded.erase( { *e1, *e2 } ); }
    }
  }
}
}  // namespace old

int main() {
  using clk = std::chrono::steady_clock;
  std::printf( "%8s %8s %14s %14s %8s\n", "entities", "moving", "old us/check", "new us/check", "speedup" );
  Entity next = 1;
  for ( unsigned n : { 32u, 64u, 128u, 255u } ) {
    for ( unsigned pct : { 5u, 25u, 100u } ) {
      std::mt19937 rng( n * 100 + pct );
      std::vector<Entity> layer;
      auto& cd = CollisionDetector::getInstance();
      auto& kin = Kinematics::getInstance();
      // A fresh set of ids per run would need more than 255; reuse 1..n, removing the last run's.
      for ( Entity e = 1; e < next; ++e ) { cd.removeEntity( e ); kin.forget( e ); }
      old::recorded.clear();
      for ( Entity e = 1; e <= n; ++e ) {
        World::set<Box>( e, Box( Position( rng() % 200, rng() % 60 ), Size( 2, 2 ) ) );
        World::set<CollBox>( e, CollBox( Box(), 1 + rng() % 5 ) );
        cd.addEntity( e, 0 );
        layer.push_back( e );
        if ( rng() % 100 < pct ) kin.setVelocity( e, Position( 0, 0 ) + Position( 1, 0 ) / 8 );  // 32/256 cell/frame
      }
      next = n + 1;
      const int frames = 3000;
      double tOld = 0, tNew = 0;
      for ( int f = 0; f < frames; ++f ) {
        kin.run();
        auto t0 = clk::now();
        old::check( layer );
        auto t1 = clk::now();
        CollisionDetector::check();
        auto t2 = clk::now();
        tOld += std::chrono::duration<double, std::micro>( t1 - t0 ).count();
        tNew += std::chrono::duration<double, std::micro>( t2 - t1 ).count();
        if ( f % 200 == 199 ) {  // wrap the movers back so they keep meeting the others
          for ( Entity e = 1; e <= n; ++e ) {
            auto& b = World::get<Box>( e );
            if ( b.pos.x > 220 * 256 ) b.pos.x -= 220 * 256;
          }
        }
      }
      std::printf( "%8u %7u%% %14.2f %14.2f %7.1fx\n", n, pct, tOld / frames, tNew / frames, tOld / tNew );
    }
  }
  std::printf( "(%zu events)\n", nEvents );
}
