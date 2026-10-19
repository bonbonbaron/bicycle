#pragma once
// Edgebreaker's active-boundary bookkeeping, shared by the decoder (src/data/EdgebreakerDecode.cpp, in the
// engine) and the encoder (tools/codec/EdgebreakerEncode.cpp, offline only) so they can't disagree.
#include "data/Edgebreaker.h"
#include <cstdint>
#include <vector>

namespace eb::detail {

// One node of an active boundary loop: the loop goes vertex -> next->vertex, and the triangle still to be
// visited across that edge contains it reversed. opp is the far vertex of the visited triangle on this edge.
struct Node {
  int32_t v{};
  int32_t opp{-1};
  int32_t prev{};
  int32_t next{};
};

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

}  // namespace eb::detail
