#include "c/CollisionDetector.h"
#include "c/Trigger.h"
#include "m/World.h"
#include <cassert>
#include <cstdint>

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
  auto& cd = getInstance();
  for ( auto& layer : cd._layers ) cd.checkLayer( layer );
  cd.undoBlockedSteps();
}

void CollisionDetector::refresh( Layer& layer, uint32_t row ) {
  const auto c = getCollBox( layer.key( row ) );
  layer.col<X0>()[row] = c.pos.x;
  layer.col<Y0>()[row] = c.pos.y;
  layer.col<X1>()[row] = c.pos.x + static_cast<int>( c.size.w );
  layer.col<Y1>()[row] = c.pos.y + static_cast<int>( c.size.h );
}

namespace {
// hits[j] = whether box a overlaps row j's box, for j in [from, to): branch-free over contiguous columns (vectorizes).
void overlapRow( const int a[4], const int* __restrict x0, const int* __restrict y0, const int* __restrict x1,
                 const int* __restrict y1, uint8_t* __restrict hits, uint32_t from, uint32_t to ) {
  for ( uint32_t j = from; j < to; ++j ) {
    hits[j] = ( a[0] < x1[j] ) & ( a[2] > x0[j] ) & ( a[1] < y1[j] ) & ( a[3] > y0[j] );
  }
}
}  // namespace

// Every active row against every row after it. Two passes per row: the overlap tests (branch-free over contiguous
// columns, so they vectorize), then the events for the pairs whose state changed.
void CollisionDetector::checkLayer( Layer& layer ) {
  const uint32_t n = layer.size(), nActive = layer.nActive();
  if ( nActive == 0 ) return;
  for ( uint32_t i = 0; i < nActive; ++i ) refresh( layer, i );  // only moving entities' boxes change
  _hits.resize( n );
  const int *x0 = layer.col<X0>(), *y0 = layer.col<Y0>(), *x1 = layer.col<X1>(), *y1 = layer.col<Y1>();
  for ( uint32_t i = 0; i < nActive; ++i ) {
    const int a[4] = { x0[i], y0[i], x1[i], y1[i] };
    overlapRow( a, x0, y0, x1, y1, _hits.data(), i + 1, n );
    const Entity e = layer.key( i );
    for ( uint32_t j = i + 1; j < n; ++j ) pairState( e, layer.key( j ), _hits[j] );
  }
}

void CollisionDetector::pairState( Entity a, Entity b, bool overlap ) {
  const size_t bit = pairBit( a, b );
  if ( overlap == _touching.test( bit ) ) return;
  const unsigned ta = World::get<CollBox>( a ).type, tb = World::get<CollBox>( b ).type;
  if ( overlap ) {
    Trigger::sendCollision( { a, b, tb } );
    Trigger::sendCollision( { b, a, ta } );
    _touching.set( bit );
    // Walked into something that blocks it: undone after this check (the collision still counts).
    if ( _stepping.test( a ) && ( _blockers[a] >> tb & 1u ) ) _undo.set( a );
    if ( _stepping.test( b ) && ( _blockers[b] >> ta & 1u ) ) _undo.set( b );
  }
  else {
    Trigger::sendUncollision( { a, b, tb } );
    Trigger::sendUncollision( { b, a, ta } );
    _touching.reset( bit );
  }
}

// Blocked steps go back to where they started. The entity stays active (Kinematics keeps a stepped entity active
// for two checks), so the next check sees it leave what it walked into.
void CollisionDetector::undoBlockedSteps() {
  for ( Entity e = 0; e < NUM_SUPPORTED_ENTITIES; ++e ) {
    if ( !_stepping.test( e ) ) continue;
    _stepBlocked.set( e, _undo.test( e ) );
    if ( _undo.test( e ) ) {
      World::get<Box>( e ).pos = _stepFrom[e];
      for ( auto& layer : _layers ) {
        if ( layer.contains( e ) ) refresh( layer, layer.row( e ) );
      }
    }
  }
  _stepping.reset();
  _undo.reset();
}

void CollisionDetector::stepped( Entity e, Position from ) {
  if ( !_stepping.test( e ) ) _stepFrom[e] = from;  // two steps in a frame: back to before the first
  _stepping.set( e );
  _stepBlocked.reset( e );
}

void CollisionDetector::setBlockers( Entity e, uint32_t mask ) { _blockers.at( e ) = mask; }

// Touching a blocker whose center lies the way it wants to go (e.g. an enemy that walked into the hero). Rows are
// about twice as tall as columns are wide, so vertical distances count 4x against the 2-column horizontal step.
auto CollisionDetector::blockedToward( Entity e, Position d ) const -> bool {
  if ( _blockers.at( e ) == 0 ) return false;
  const auto& me = World::get<Box>( e );
  for ( Entity o = 0; o < NUM_SUPPORTED_ENTITIES; ++o ) {
    if ( o == e || !touching( e, o ) || !( _blockers[e] >> World::get<CollBox>( o ).type & 1u ) ) continue;
    const auto& b = World::get<Box>( o );
    // Twice the center-to-center distance (no halving, so no rounding).
    const int64_t dx = ( 2ll * b.pos.x + b.size.w ) - ( 2ll * me.pos.x + me.size.w );
    const int64_t dy = ( 2ll * b.pos.y + b.size.h ) - ( 2ll * me.pos.y + me.size.h );
    if ( d.x * dx + d.y * dy * 4 > 0 ) return true;
  }
  return false;
}

void CollisionDetector::addEntity( Entity entity, unsigned layer ) {
  auto& l = _layers.at( layer );
  if ( !l.add( entity, 0, 0, 0, 0 ) ) return;
  refresh( l, l.row( entity ) );
  _layersOf.at( entity ) |= 1u << layer;
}

void CollisionDetector::registerEntity( Entity entity, unsigned layer ) {
  assert( layer < NUM_SUPPORTED_COLL_LAYERS );
  getInstance().addEntity( entity, layer );
}

void CollisionDetector::activate( Entity e ) {
  if ( e >= NUM_SUPPORTED_ENTITIES ) return;
  for ( unsigned l = 0; l < NUM_SUPPORTED_COLL_LAYERS; ++l ) {
    if ( _layersOf[e] >> l & 1u ) _layers[l].activate( e );
  }
}

// Its box is final for as long as it's still: record it now.
void CollisionDetector::deactivate( Entity e ) {
  if ( e >= NUM_SUPPORTED_ENTITIES ) return;
  for ( unsigned l = 0; l < NUM_SUPPORTED_COLL_LAYERS; ++l ) {
    if ( _layersOf[e] >> l & 1u && _layers[l].deactivate( e ) ) refresh( _layers[l], _layers[l].row( e ) );
  }
}

auto CollisionDetector::isActive( Entity e ) const -> bool {
  for ( const auto& l : _layers ) {
    if ( l.isActive( e ) ) return true;
  }
  return false;
}

// Whoever was touching the entity hears an uncollision (as if it walked away), so nobody keeps a stale contact.
void CollisionDetector::removeEntity( Entity entity ) {
  if ( entity >= NUM_SUPPORTED_ENTITIES ) return;
  for ( auto& layer : _layers ) layer.remove( entity );
  _layersOf[entity] = 0;
  const auto type = World::get<CollBox>( entity ).type;
  for ( Entity other = 0; other < NUM_SUPPORTED_ENTITIES; ++other ) {
    if ( other != entity && touching( entity, other ) ) {
      Trigger::sendUncollision( { other, entity, type } );
      _touching.reset( pairBit( entity, other ) );
    }
  }
  _blockers[entity] = 0;
  _stepping.reset( entity );
  _stepBlocked.reset( entity );
  _undo.reset( entity );
}
