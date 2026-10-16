#include <iterator>
#include <cassert>
#include "c/CollisionDetector.h"
#include "c/Trigger.h"
#include "m/World.h"

/* What kind of mage do I want to be?
       * One who understands, but not one who philosophizes.
       * One who gets tools out quickly.
       * One who's result-oriented.
       * One who serves others rather than his own ego (e.g. the idol of own's own cleverness).
       */

// It's wise to give each tile control over its own dimension.
// Because then there are fewer collisions to check in bigger tiles.
// However, that erases the meaning of "number of tiles high/wide".
// What this means is getting the smallest tile dim, or rather, a flat-out 1x1 tile size, and creating a grid on that.
// An entity's collision box is its CollBox offset by its position, or its whole box if the CollBox is empty.
static auto getCollBox( const Entity entity ) -> CollBox {
  const auto& box = World::get<Box>( entity );
  auto coll = World::get<CollBox>( entity );
  if ( coll.size.w == 0 || coll.size.h == 0 ) {
    coll.size = box.size;
    coll.pos = Position{};
  }
  coll.pos += box.pos;
  return coll;
}

auto CollisionDetector::getInstance() -> CollisionDetector& {
  static CollisionDetector cd;
  return cd;
}

void CollisionDetector::check() {
  auto& trig = Trigger::getInstance();
  auto& cd = getInstance();
  auto collLayers = cd.getCollisionLayers();
  // TODO Background collision detection
  for ( const auto& collLayer : collLayers ) {
    for ( auto e1 = collLayer.begin(); e1 != collLayer.end(); ++e1 ) {
      /* TODO bg element width & height to compute which cells you overlap
              Does that require a structure? Like a bg tiledim, width, and height structure?
       */
    }
  }
  // Foreground collision detection
  for ( const auto& collLayer : collLayers ) {
    for ( auto e1 = collLayer.begin(); e1 != collLayer.end(); ++e1 ) {
      const auto f1 = getCollBox( *e1 );
      for ( auto e2 = std::next(e1); e2 != collLayer.end(); ++e2 ) {
				auto wereCollided = cd.recordedCollision( *e1, *e2 );
        const auto f2 = getCollBox( *e2 );
        if ( f1.overlaps( f2 ) ) {
          if ( !wereCollided ) {
            // TODO send both collisions in one function call by including both types
            trig.sendCollision( { *e1, *e2, f2.type } );
            trig.sendCollision( { *e2, *e1, f1.type } );
            cd.recordCollision( *e1, *e2 );
          }
        }
        else if ( wereCollided ) {
          trig.sendUncollision( { *e1, *e2, f2.type } );
          trig.sendUncollision( { *e2, *e1, f1.type } );
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

// Whoever was touching the entity hears an uncollision (as if it walked away), so nobody keeps a stale contact.
void CollisionDetector::removeEntity( Entity entity ) {
  for ( auto& layer : _collLayers ) {
    std::erase( layer, entity );
  }
  const auto type = World::get<CollBox>( entity ).type;
  for ( auto it = _recordedCollisions.begin(); it != _recordedCollisions.end(); ) {
    if ( it->first == entity || it->second == entity ) {
      const Entity other = it->first == entity ? it->second : it->first;
      Trigger::sendUncollision( { other, entity, type } );
      it = _recordedCollisions.erase( it );
    }
    else {
      ++it;
    }
  }
}
