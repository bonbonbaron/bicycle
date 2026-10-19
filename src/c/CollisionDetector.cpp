#include "c/CollisionDetector.h"
#include "c/Trigger.h"
#include "c/Kinematics.h"
#include "c/ImageOverlaps.h"
#include "m/World.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>

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

namespace {
// Edges of a collision box (X1/Y1 exclusive).
struct Edges {
  int x0, y0, x1, y1;
  explicit Edges( const CollBox& c )
      : x0( c.pos.x ), y0( c.pos.y ), x1( c.pos.x + static_cast<int>( c.size.w ) ),
        y1( c.pos.y + static_cast<int>( c.size.h ) ) {}
  auto xOverlap( const Edges& o ) const -> bool { return x0 < o.x1 && o.x0 < x1; }
  auto yOverlap( const Edges& o ) const -> bool { return y0 < o.y1 && o.y0 < y1; }
  auto overlaps( const Edges& o ) const -> bool { return xOverlap( o ) && yOverlap( o ); }
  // Flush: one pair of edges meets while the other axis overlaps (corners meeting don't count).
  auto flush( const Edges& o ) const -> bool {
    return ( ( x1 == o.x0 || o.x1 == x0 ) && yOverlap( o ) ) || ( ( y1 == o.y0 || o.y1 == y0 ) && xOverlap( o ) );
  }
};
}  // namespace

auto CollisionDetector::getInstance() -> CollisionDetector& {
  static CollisionDetector cd;
  return cd;
}

void CollisionDetector::check() {
  auto& cd = getInstance();
  for ( auto& layer : cd._layers ) cd.checkLayer( layer );
  for ( const auto& [mover, blocker] : cd._pushes ) cd.pushOut( mover, blocker );
  cd._pushes.clear();
  cd._moved.reset();
  cd._stepping.reset();
  imageOverlaps::update();  // 2D and ASCII: which drawn images overlap (depth sorting; ImageOverlaps.h)
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
    for ( uint32_t j = i + 1; j < n; ++j ) {
      const Entity o = layer.key( j );
      pairState( e, o, _hits[j] );
      if ( !_hits[j] ) continue;
      // Moved into something that blocks it: pushed back out after the checks.
      if ( _moved.test( e ) && ( _blockers[e] >> World::get<CollBox>( o ).type & 1u ) ) _pushes.emplace_back( e, o );
      if ( _moved.test( o ) && ( _blockers[o] >> World::get<CollBox>( e ).type & 1u ) ) _pushes.emplace_back( o, e );
    }
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
  }
  else {
    // Pushed flush against a blocker it's still touching (e.g. walking along a wall).
    if ( blocking( a, b ) && Edges( getCollBox( a ) ).flush( Edges( getCollBox( b ) ) ) ) return;
    Trigger::sendUncollision( { a, b, tb } );
    Trigger::sendUncollision( { b, a, ta } );
    _touching.reset( bit );
  }
}

auto CollisionDetector::blocking( Entity a, Entity b ) const -> bool {
  return ( _blockers[a] >> World::get<CollBox>( b ).type & 1u ) || ( _blockers[b] >> World::get<CollBox>( a ).type & 1u );
}

// The mover overlaps the blocker after moving by d = pos - from. It came in on the axis that was still apart before
// the move (if both were, diagonally, the one that closed last: the larger gap per unit of motion). On that axis it
// goes back just far enough to be flush, and its velocity loses that component if it points into the blocker; the
// other axis keeps its motion, so it slides. Overlapping before the move too (e.g. placed inside), it's left alone.
void CollisionDetector::pushOut( Entity m, Entity o ) {
  auto& pos = World::get<Box>( m ).pos;
  const Edges me( getCollBox( m ) ), it( getCollBox( o ) );
  if ( !me.overlaps( it ) ) return;  // an earlier push already got it out
  const Position d = pos - _from[m];
  const Edges was{ CollBox( Box( getCollBox( m ).pos - d, getCollBox( m ).size ), 0 ) };
  const bool apartX = d.x != 0 && !was.xOverlap( it ), apartY = d.y != 0 && !was.yOverlap( it );
  if ( !apartX && !apartY ) return;
  bool alongX = apartX;
  if ( apartX && apartY ) {
    const int64_t gapX = d.x > 0 ? it.x0 - was.x1 : was.x0 - it.x1;
    const int64_t gapY = d.y > 0 ? it.y0 - was.y1 : was.y0 - it.y1;
    alongX = gapX * std::abs( d.y ) >= gapY * std::abs( d.x );
  }
  auto& kin = Kinematics::getInstance();
  if ( alongX ) {
    pos.x += d.x > 0 ? it.x0 - me.x1 : it.x1 - me.x0;
    kin.stopToward( m, Position( d.x > 0 ? 1 : -1, 0 ) );
  }
  else {
    pos.y += d.y > 0 ? it.y0 - me.y1 : it.y1 - me.y0;
    kin.stopToward( m, Position( 0, d.y > 0 ? 1 : -1 ) );
  }
  if ( _stepping.test( m ) ) _stepBlocked.set( m );
  for ( auto& layer : _layers ) {
    if ( layer.contains( m ) ) refresh( layer, layer.row( m ) );
  }
  kin.moved( m );  // checked again next frame from where it ended up
}

void CollisionDetector::movedFrom( Entity e, Position from, bool step ) {
  if ( !_moved.test( e ) ) _from[e] = from;  // moved twice in a frame: from before the first
  _moved.set( e );
  if ( step ) {
    _stepping.set( e );
    _stepBlocked.reset( e );
  }
}

void CollisionDetector::setBlockers( Entity e, uint32_t mask ) { _blockers.at( e ) = mask; }

// Drops the parts of a step that go further into a blocker it touches: flush against it, or overlapping it with the
// blocker's near edge on that side (something walked into it). The rest (sliding, backing away) stays.
auto CollisionDetector::unblocked( Entity e, Position d ) const -> Position {
  if ( _blockers.at( e ) == 0 ) return d;
  const Edges me( getCollBox( e ) );
  for ( Entity o = 0; o < NUM_SUPPORTED_ENTITIES; ++o ) {
    if ( o == e || !touching( e, o ) || !( _blockers[e] >> World::get<CollBox>( o ).type & 1u ) ) continue;
    const Edges it( getCollBox( o ) );
    const auto ahead = []( int lo, int edge, int hi ) { return lo < edge && edge <= hi; };  // edge in (lo, hi]
    if ( me.yOverlap( it ) && ( ( d.x > 0 && ahead( me.x0, it.x0, me.x1 ) ) || ( d.x < 0 && ahead( -me.x1, -it.x1, -me.x0 ) ) ) ) d.x = 0;
    if ( me.xOverlap( it ) && ( ( d.y > 0 && ahead( me.y0, it.y0, me.y1 ) ) || ( d.y < 0 && ahead( -me.y1, -it.y1, -me.y0 ) ) ) ) d.y = 0;
  }
  return d;
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
  imageOverlaps::moving( e, true );
  for ( unsigned l = 0; l < NUM_SUPPORTED_COLL_LAYERS; ++l ) {
    if ( _layersOf[e] >> l & 1u ) _layers[l].activate( e );
  }
}

// Its box is final for as long as it's still: record it now.
void CollisionDetector::deactivate( Entity e ) {
  if ( e >= NUM_SUPPORTED_ENTITIES ) return;
  imageOverlaps::moving( e, false );
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
  imageOverlaps::forget( entity );
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
  _moved.reset( entity );
  _stepping.reset( entity );
  _stepBlocked.reset( entity );
  std::erase_if( _pushes, [entity]( const auto& p ) { return p.first == entity || p.second == entity; } );
}
