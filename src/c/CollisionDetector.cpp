#include <iterator>
#include <cassert>

#include "c/CollisionDetector.h"
#include "c/Trigger.h"
#include "m/World.h"

auto CollisionDetector::getInstance() -> CollisionDetector& {
  static CollisionDetector cd;
  return cd;
}

void CollisionDetector::check() {
  // TODO also check for collisions with BG tiles
  auto& trig = Trigger::getInstance();
  auto& cd = getInstance();
  auto collLayers = cd.getCollisionLayers();
  for ( const auto& collLayer : collLayers ) {
    for ( auto e1 = collLayer.begin(); e1 != collLayer.end(); ++e1 ) {
      const auto& r1 = World::get<Rect>( *e1 );
      auto& c1 = World::get<CollRect>( *e1 );
      auto f1 = c1;
      f1.pos += r1.pos;
      for ( auto e2 = std::next(e1); e2 != collLayer.end(); ++e2 ) {
				auto wereCollided = cd.recordedCollision( *e1, *e2 );
        const auto& r2 = World::get<Rect>( *e2 );
        auto& c2 = World::get<CollRect>( *e2 );
        auto f2 = c2;
        f2.pos += r2.pos;
        if ( r1.overlaps( r2 ) ) {
          if ( !wereCollided ) {
            trig.onCollision( { *e1, *e2, f2.type } );
            trig.onCollision( { *e2, *e1, f1.type } );
            cd.recordCollision( *e1, *e2 );
          }
        }
        else if ( wereCollided ) {
          // TODO trig.offCollision( { *e1, *e2 } );
          cd.deleteCollision( *e1, *e2 );
        }
      }  // for each entity for *e1 to check for collisions with
    }  // for each entity on this collision layer
  }  // for each collision layer
}

void CollisionDetector::recordCollision( Entity e1, Entity e2 ) {
	_recordedCollisions.insert( { e1, e2 } );
}

void CollisionDetector::deleteCollision( Entity e1, Entity e2 ) {
	_recordedCollisions.erase( { e1, e2 } );
}

// The smaller entity ID is always on the LHS, so this always works.
auto CollisionDetector::recordedCollision( Entity e1, Entity e2 ) -> bool {
	return _recordedCollisions.contains( { e1, e2 } );
}

auto CollisionDetector::getCollisionLayers() -> CollisionLayers& {
	return _collLayers;
}

void CollisionDetector::addEntity( Entity entity, unsigned layer ) {
	_collLayers.at( layer ).push_back( entity );
}

void CollisionDetector::registerEntity( Entity entity, unsigned layer ) {
	assert( layer < NUM_SUPPORTED_COLL_LAYERS );
	auto& cd = getInstance();
	cd.addEntity( entity, layer );
}
