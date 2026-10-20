#include "c/ImageOverlaps.h"
#include "m/World.h"
#include <numeric>

namespace imageOverlaps {
void drawnOn( Entity e, Entity layer ) { ImageOverlaps::getInstance().add( e, layer ); }
void moving( Entity e, bool on ) { ImageOverlaps::getInstance().setMoving( e, on ); }
void forget( Entity e ) { ImageOverlaps::getInstance().remove( e ); }
void update() { ImageOverlaps::getInstance().update(); }
}  // namespace imageOverlaps

auto ImageOverlaps::getInstance() -> ImageOverlaps& {
  static ImageOverlaps io;
  return io;
}

void ImageOverlaps::refresh( uint32_t row ) {
  const auto& b = World::get<Box>( _rects.key( row ) );
  _rects.col<X0>()[row] = b.pos.x;
  _rects.col<Y0>()[row] = b.pos.y;
  _rects.col<X1>()[row] = b.pos.x + static_cast<int>( b.size.w );
  _rects.col<Y1>()[row] = b.pos.y + static_cast<int>( b.size.h );
}

// A new member is checked against the rest on the next update; one already drawn elsewhere moves layers.
void ImageOverlaps::add( Entity e, Entity layer ) {
  if ( e >= NUM_SUPPORTED_ENTITIES || layer >= NUM_SUPPORTED_ENTITIES ) return;
  remove( e );
  _rects.add( e, 0, 0, 0, 0, layer );
  _rects.activate( e );
}

void ImageOverlaps::remove( Entity e ) {
  if ( e >= NUM_SUPPORTED_ENTITIES || !_rects.contains( e ) ) return;
  const Entity layer = _rects.at<LAYER>( e );
  for ( Entity o = 0; o < NUM_SUPPORTED_ENTITIES; ++o ) setOverlap( e, o, false, layer );
  _rects.remove( e );
  _moving.reset( e );
  _group[e] = NONE;
  if ( _dirty.test( layer ) ) regroup( layer );
}

void ImageOverlaps::setMoving( Entity e, bool on ) {
  if ( e >= NUM_SUPPORTED_ENTITIES ) return;
  _moving.set( e, on );
  _rects.activate( e );  // stopping counts too: its last box is checked once more
}

void ImageOverlaps::setOverlap( Entity a, Entity b, bool on, Entity layer ) {
  const std::size_t ab = a * NUM_SUPPORTED_ENTITIES + b;
  if ( _overlap.test( ab ) == on ) return;
  _overlap.set( ab, on );
  _overlap.set( b * NUM_SUPPORTED_ENTITIES + a, on );
  _dirty.set( layer );
}

namespace {
// hits[j] = whether rect a overlaps row j's rect in the same layer: branch-free over contiguous columns (vectorizes).
/*
  In C++, __restrict (or __restrict__) is a compiler-specific extension rather than a standard C++ keyword. 
  It is used as a pointer optimization hint to tell the compiler that the pointer is the sole means of accessing 
  a given memory region within its scope. By eliminating the compiler's worry about pointer aliasing (where two 
  pointers could point to overlapping memory locations), the optimizer can generate much cleaner assembly, avoid 
  redundant memory reloads, and easily apply loop vectorization.
*/
void overlapRow( const int a[4], Entity layer, const int* __restrict x0, const int* __restrict y0,
                 const int* __restrict x1, const int* __restrict y1, const Entity* __restrict layers,
                 uint8_t* __restrict hits, uint32_t n ) {
  for ( uint32_t j = 0; j < n; ++j ) {
    hits[j] = ( a[0] < x1[j] ) & ( a[2] > x0[j] ) & ( a[1] < y1[j] ) & ( a[3] > y0[j] ) & ( layers[j] == layer );
  }
}
}  // namespace

// The active rows' rects against every row; layers whose overlaps changed are regrouped. Rows that stopped moving
// go inactive (backwards: deactivating swaps a row with the last active one, which was already visited).
void ImageOverlaps::update() {
  const uint32_t n = _rects.size(), nActive = _rects.nActive();
  if ( nActive == 0 ) return;
  for ( uint32_t i = 0; i < nActive; ++i ) refresh( i );
  _hits.resize( n );
  const int *x0 = _rects.col<X0>(), *y0 = _rects.col<Y0>(), *x1 = _rects.col<X1>(), *y1 = _rects.col<Y1>();
  const Entity* layers = _rects.col<LAYER>();
  for ( uint32_t i = 0; i < nActive; ++i ) {
    const int a[4] = { x0[i], y0[i], x1[i], y1[i] };
    overlapRow( a, layers[i], x0, y0, x1, y1, layers, _hits.data(), n );
    _hits[i] = 0;  // not with itself
    const Entity e = _rects.key( i );
    for ( uint32_t j = 0; j < n; ++j ) setOverlap( e, _rects.key( j ), _hits[j], layers[i] );
  }
  for ( uint32_t i = nActive; i-- > 0; ) {
    if ( !_moving.test( _rects.key( i ) ) ) _rects.deactivate( _rects.key( i ) );
  }
  for ( Entity l = 0; l < NUM_SUPPORTED_ENTITIES; ++l ) {
    if ( _dirty.test( l ) ) regroup( l );
  }
}

// Connected components of the layer's overlaps (union-find); a member's group is its component's root unless it's
// alone in it.
void ImageOverlaps::regroup( Entity layer ) {
  _dirty.reset( layer );
  std::vector<Entity> members;
  for ( uint32_t r = 0; r < _rects.size(); ++r ) {
    if ( _rects.col<LAYER>()[r] == layer ) members.push_back( _rects.key( r ) );
  }
  std::array<Entity, NUM_SUPPORTED_ENTITIES> parent;
  for ( const Entity e : members ) parent[e] = e;
  const auto root = [&parent]( Entity e ) {
    while ( parent[e] != e ) e = parent[e] = parent[parent[e]];
    return e;
  };
  for ( std::size_t i = 0; i < members.size(); ++i ) {
    for ( std::size_t j = i + 1; j < members.size(); ++j ) {
      if ( overlapping( members[i], members[j] ) ) parent[root( members[i] )] = root( members[j] );
    }
  }
  std::array<uint16_t, NUM_SUPPORTED_ENTITIES> size{};
  for ( const Entity e : members ) ++size[root( e )];
  bool any = false;
  for ( const Entity e : members ) {
    const Entity r = root( e );
    _group[e] = size[r] > 1 ? r : NONE;
    any |= size[r] > 1;
  }
  _layerGrouped.set( layer, any );
}
