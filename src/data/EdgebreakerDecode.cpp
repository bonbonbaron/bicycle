// Edgebreaker decoding (runtime). The encoder is offline only: tools/codec/EdgebreakerEncode.cpp.
#include "data/Edgebreaker.h"
#include "EdgebreakerLoops.h"

namespace eb {

using detail::Loops;

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
  std::size_t opIdx = 0, sIdx = 0;
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
