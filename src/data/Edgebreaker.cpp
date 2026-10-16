#include "data/Edgebreaker.h"
#include <algorithm>
#include <unordered_map>

namespace eb {

namespace {

// One node of an active boundary loop: the loop goes vertex -> next->vertex, and the triangle still to be
// visited across that edge contains it reversed. opp is the far vertex of the visited triangle on this edge.
struct Node {
  int32_t v{};
  int32_t opp{-1};
  int32_t prev{};
  int32_t next{};
};

// The loop bookkeeping shared by the encoder and the decoder, so they can't disagree.
struct Loops {
  std::vector<Node> nodes;
  std::vector<int32_t> stack;  // gates of loops waiting their turn
  auto add( int32_t v, int32_t opp ) -> int32_t {
    nodes.push_back( { v, opp, 0, 0 } );
    return static_cast<int32_t>( nodes.size() ) - 1;
  }
  void link( int32_t a, int32_t b ) { nodes[a].next = b; nodes[b].prev = a; }
  // First triangle of an island: (a, b, c) counter-clockwise.
  auto start( int32_t a, int32_t b, int32_t c ) -> int32_t {
    const int32_t na = add( a, c ), nb = add( b, a ), nc = add( c, b );
    link( na, nb ); link( nb, nc ); link( nc, na );
    return na;
  }
  auto length( int32_t g ) const -> int {
    int n = 1;
    for ( int32_t i = nodes[g].next; i != g; i = nodes[i].next ) ++n;
    return n;
  }
  // Applies op to gate g (u = g.v, v = g.next.v) whose tip is w; returns the next gate or -1 when the loop is done.
  auto apply( Op op, int32_t g, int32_t w, uint32_t offset ) -> int32_t {
    const int32_t gn = nodes[g].next;
    const int32_t u = nodes[g].v, v = nodes[gn].v;
    switch ( op ) {
      case C: case D: {
        const int32_t nw = add( w, u );  // edge w -> v; its visited triangle is (v, u, w)
        nodes[g].opp = v;                // edge u -> w
        link( g, nw ); link( nw, gn );
        return nw;
      }
      case R: {  // tip is after v: v drops off the loop
        const int32_t nw = nodes[gn].next;
        link( g, nw );
        nodes[g].opp = v;
        return g;
      }
      case L: {  // tip is before u: u drops off
        const int32_t p = nodes[g].prev;
        link( p, gn );
        nodes[p].opp = u;
        return p;
      }
      case E:
        return -1;
      case S: {
        int32_t k = gn;
        for ( uint32_t i = 0; i < offset; ++i ) k = nodes[k].next;
        const int32_t after = nodes[k].next;
        const int32_t x = add( nodes[k].v, nodes[k].opp );  // w's copy in the loop that keeps the gate
        // Loop B: v ... w, closed by the new edge w -> v.
        link( k, gn );
        nodes[k].opp = u;
        // Loop A: u -> w -> (what followed w) ... -> u.
        link( g, x );
        link( x, after );
        nodes[g].opp = v;
        stack.push_back( k );
        return g;
      }
    }
    return -1;
  }
  auto pop() -> int32_t {
    if ( stack.empty() ) return -1;
    const int32_t g = stack.back();
    stack.pop_back();
    return g;
  }
};

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

auto decode( const Connectivity& conn, Decoded& out ) -> bool {
  out = Decoded{};
  if ( conn.raw ) {
    out.nVerts = conn.nVerts;
    out.tris = conn.rawTris;
    out.dummy.assign( conn.nVerts, 0 );
    out.pred.assign( conn.nVerts, { -1, -1, -1 } );
    for ( auto v : out.tris ) if ( v >= conn.nVerts ) return false;
    return out.tris.size() % 3 == 0;
  }
  Loops loops;
  auto newVertex = [&]( bool dummy, std::array<int32_t, 3> pred ) -> int32_t {
    out.dummy.push_back( dummy );
    out.pred.push_back( pred );
    return static_cast<int32_t>( out.nVerts++ );
  };
  size_t opIdx = 0, sIdx = 0;
  int32_t g = -1;
  while ( opIdx < conn.ops.size() ) {
    if ( g < 0 ) {  // new island
      const int32_t a = newVertex( false, { -1, -1, -1 } );
      const int32_t b = newVertex( false, { a, -1, -1 } );
      const int32_t c = newVertex( false, { a, b, -1 } );
      out.tris.insert( out.tris.end(), { static_cast<uint32_t>( a ), static_cast<uint32_t>( b ), static_cast<uint32_t>( c ) } );
      g = loops.start( a, b, c );
    }
    const Op op = static_cast<Op>( conn.ops[opIdx++] );
    const int32_t gn = loops.nodes[g].next;
    const int32_t u = loops.nodes[g].v, v = loops.nodes[gn].v;
    int32_t w = -1;
    uint32_t offset = 0;
    switch ( op ) {
      case C: case D:
        w = newVertex( op == D, { u, v, loops.nodes[g].opp } );
        break;
      case R: w = loops.nodes[loops.nodes[gn].next].v; break;
      case L: w = loops.nodes[loops.nodes[g].prev].v; break;
      case E:
        if ( loops.length( g ) != 3 ) return false;
        w = loops.nodes[loops.nodes[gn].next].v;
        break;
      case S: {
        if ( sIdx >= conn.offsets.size() ) return false;
        offset = conn.offsets[sIdx++];
        const int len = loops.length( g );
        if ( offset < 2 || static_cast<int>( offset ) > len - 3 ) return false;
        int32_t k = gn;
        for ( uint32_t i = 0; i < offset; ++i ) k = loops.nodes[k].next;
        w = loops.nodes[k].v;
        break;
      }
      default:
        return false;
    }
    out.tris.insert( out.tris.end(), { static_cast<uint32_t>( v ), static_cast<uint32_t>( u ), static_cast<uint32_t>( w ) } );
    g = loops.apply( op, g, w, offset );
    if ( g < 0 ) g = loops.pop();
  }
  return out.nVerts == conn.nVerts && sIdx == conn.offsets.size();
}

}  // namespace eb
