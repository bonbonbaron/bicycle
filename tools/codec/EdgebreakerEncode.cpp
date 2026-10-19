// Edgebreaker encoding (offline: bicycle-pack and the codec tests; the engine only decodes, see
// src/data/EdgebreakerDecode.cpp).
#include "codec/EdgebreakerEncode.h"
#include "../../src/data/EdgebreakerLoops.h"
#include <algorithm>
#include <unordered_map>

namespace eb {

namespace {

using detail::Loops;

auto rawFallback( const std::vector<uint32_t>& tris, uint32_t nVerts ) -> Encoded {
  Encoded e;
  e.conn.raw = true;
  e.conn.nVerts = nVerts;
  e.conn.rawTris = tris;
  e.vertexOrigin.resize( nVerts );
  for ( uint32_t i = 0; i < nVerts; ++i ) e.vertexOrigin[i] = static_cast<int32_t>( i );
  e.triOrigin.resize( tris.size() / 3 );
  for ( size_t i = 0; i < e.triOrigin.size(); ++i ) e.triOrigin[i] = static_cast<int32_t>( i );
  e.triRotation.assign( tris.size() / 3, 0 );
  return e;
}

auto key( uint32_t a, uint32_t b ) -> uint64_t {
  return ( static_cast<uint64_t>( a ) << 32 ) | b;
}

}  // namespace

auto encode( const std::vector<uint32_t>& inTris, uint32_t nVerts ) -> Encoded {
  const size_t nReal = inTris.size() / 3;
  std::vector<uint32_t> tris = inTris;
  // Directed edges -> triangle. A repeated directed edge means non-manifold or inconsistent winding.
  std::unordered_map<uint64_t, uint32_t> edgeTri;
  edgeTri.reserve( tris.size() * 2 );
  for ( size_t t = 0; t < nReal; ++t ) {
    for ( int j = 0; j < 3; ++j ) {
      const uint32_t a = tris[3 * t + j], b = tris[3 * t + ( j + 1 ) % 3];
      if ( a == b || !edgeTri.emplace( key( a, b ), static_cast<uint32_t>( t ) ).second ) return rawFallback( inTris, nVerts );
    }
  }
  // Close each hole with a dummy vertex: a boundary edge a -> b gets the triangle (b, a, dummy).
  std::unordered_map<uint32_t, uint32_t> boundaryNext;  // b -> a for boundary edges a -> b reversed: walk the hole
  for ( auto& [k, t] : edgeTri ) {
    const uint32_t a = static_cast<uint32_t>( k >> 32 ), b = static_cast<uint32_t>( k );
    if ( !edgeTri.count( key( b, a ) ) ) {
      if ( !boundaryNext.emplace( b, a ).second ) return rawFallback( inTris, nVerts );  // pinched boundary
    }
  }
  uint32_t nAll = nVerts;
  std::vector<uint8_t> isDummy( nVerts, 0 );
  {
    std::unordered_map<uint32_t, bool> done;
    // Deterministic order: walk boundary vertices in increasing id.
    std::vector<uint32_t> starts;
    for ( auto& [b, a] : boundaryNext ) starts.push_back( b );
    std::sort( starts.begin(), starts.end() );
    for ( uint32_t s : starts ) {
      if ( done[s] ) continue;
      const uint32_t dummy = nAll++;
      isDummy.push_back( 1 );
      uint32_t b = s;
      int guard = 0;
      do {
        done[b] = true;
        const uint32_t a = boundaryNext.at( b );   // boundary edge a -> b
        tris.insert( tris.end(), { b, a, dummy } );
        b = a;
        if ( ++guard > static_cast<int>( nVerts ) + 1 || !boundaryNext.count( b ) ) return rawFallback( inTris, nVerts );
      } while ( b != s );
    }
  }
  const size_t nTris = tris.size() / 3;
  for ( size_t t = nReal; t < nTris; ++t ) {
    for ( int j = 0; j < 3; ++j ) {
      if ( !edgeTri.emplace( key( tris[3 * t + j], tris[3 * t + ( j + 1 ) % 3] ), static_cast<uint32_t>( t ) ).second ) {
        return rawFallback( inTris, nVerts );
      }
    }
  }
  // Now every directed edge must have its twin.
  for ( auto& [k, t] : edgeTri ) {
    if ( !edgeTri.count( key( static_cast<uint32_t>( k ), static_cast<uint32_t>( k >> 32 ) ) ) ) return rawFallback( inTris, nVerts );
  }

  Encoded e;
  e.conn.nVerts = 0;
  std::vector<int32_t> newId( nAll, -1 );        // input vertex -> decoded id
  std::vector<uint8_t> triDone( nTris, 0 );
  auto emitVertex = [&]( uint32_t v ) {
    newId[v] = static_cast<int32_t>( e.vertexOrigin.size() );
    e.vertexOrigin.push_back( isDummy[v] ? -1 : static_cast<int32_t>( v ) );
  };
  // Records triangle t as visited, starting at corner whose vertex is first.
  auto emitTri = [&]( uint32_t t, uint32_t first ) {
    triDone[t] = 1;
    uint8_t rot = 0;
    while ( tris[3 * t + rot] != first ) ++rot;
    e.triOrigin.push_back( t < nReal ? static_cast<int32_t>( t ) : -1 );
    e.triRotation.push_back( rot );
  };
  Loops loops;  // loop vertices are INPUT ids in the encoder
  for ( size_t t0 = 0; t0 < nReal; ++t0 ) {
    if ( triDone[t0] ) continue;
    const uint32_t a = tris[3 * t0], b = tris[3 * t0 + 1], c = tris[3 * t0 + 2];
    if ( newId[a] >= 0 || newId[b] >= 0 || newId[c] >= 0 ) return rawFallback( inTris, nVerts );  // pinched islands
    emitVertex( a ); emitVertex( b ); emitVertex( c );
    emitTri( static_cast<uint32_t>( t0 ), a );
    int32_t g = loops.start( a, b, c );
    while ( g >= 0 ) {
      const int32_t gn = loops.nodes[g].next;
      const uint32_t u = loops.nodes[g].v, v = loops.nodes[gn].v;
      const auto it = edgeTri.find( key( v, u ) );
      const uint32_t t = it->second;
      if ( triDone[t] ) return rawFallback( inTris, nVerts );
      uint8_t j = 0;
      while ( tris[3 * t + j] != v ) ++j;
      const uint32_t w = tris[3 * t + ( j + 2 ) % 3];
      emitTri( t, v );  // decoded triangle is (v, u, w)
      Op op;
      uint32_t offset = 0;
      if ( newId[w] < 0 ) {
        op = isDummy[w] ? D : C;
        emitVertex( w );
      }
      else {
        const int32_t after = loops.nodes[gn].next;
        const int32_t before = loops.nodes[g].prev;
        const bool isR = static_cast<uint32_t>( loops.nodes[after].v ) == w;
        const bool isL = static_cast<uint32_t>( loops.nodes[before].v ) == w;
        if ( isR && isL && loops.length( g ) == 3 ) op = E;
        else if ( isR ) op = R;
        else if ( isL ) op = L;
        else {
          // Find w further along this loop.
          uint32_t k = 0;
          int32_t i = gn;
          bool found = false;
          for ( ; i != g; i = loops.nodes[i].next, ++k ) {
            if ( static_cast<uint32_t>( loops.nodes[i].v ) == w ) { found = true; break; }
          }
          if ( !found || k < 2 ) return rawFallback( inTris, nVerts );  // w is on another loop: a handle
          op = S;
          offset = k;
          e.conn.offsets.push_back( k );
        }
      }
      e.conn.ops.push_back( op );
      g = loops.apply( op, g, static_cast<int32_t>( w ), offset );
      if ( g < 0 ) g = loops.pop();
    }
  }
  for ( size_t t = 0; t < nTris; ++t ) {
    if ( !triDone[t] ) return rawFallback( inTris, nVerts );
  }
  e.conn.nVerts = static_cast<uint32_t>( e.vertexOrigin.size() );
  // Self-check: decoding must reproduce the triangles we just walked.
  Decoded d;
  if ( !decode( e.conn, d ) || d.tris.size() != nTris * 3 ) return rawFallback( inTris, nVerts );
  for ( size_t k = 0; k < nTris; ++k ) {
    // Map decoded triangle back and compare.
    for ( int j = 0; j < 3; ++j ) {
      const int32_t dv = static_cast<int32_t>( d.tris[3 * k + j] );
      const int32_t orig = e.triOrigin[k];
      if ( orig < 0 ) continue;
      const uint32_t want = inTris[3 * orig + ( j + e.triRotation[k] ) % 3];
      if ( e.vertexOrigin[dv] != static_cast<int32_t>( want ) ) return rawFallback( inTris, nVerts );
    }
  }
  return e;
}

}  // namespace eb
