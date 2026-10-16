// bicycle::map (include/data/Map.h): the correctness tests of /workspace/mapbench/test_map2.c (overwrite, key
// 255 / all 256 keys, exact/grow/shrink, random ops vs a reference array) plus non-trivial values, iteration,
// copies/moves and pool accounting. `ctest -R map`.
#include "data/Map.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
using namespace mem;
using bicycle::map;

static int failures = 0;
#define CHECK( c ) do { if ( !( c ) ) { ++failures; std::printf( "FAIL %s:%d %s\n", __FILE__, __LINE__, #c ); } } while ( 0 )

template <size_t N>
struct Val {  // N bytes; first 4 hold the value, the last byte repeats its low byte
  unsigned char b[N];
  static auto make( unsigned v ) -> Val {
    Val x;
    std::memset( x.b, static_cast<unsigned char>( v ), N );
    std::memcpy( x.b, &v, 4 );
    return x;
  }
  auto v() const -> unsigned { unsigned r; std::memcpy( &r, b, 4 ); return r; }
};

template <size_t N, PoolId P>
static void verify( const map<Val<N>, P>& m, const bool* has, const unsigned* ref ) {
  size_t n = 0;
  for ( int k = 0; k < 256; ++k ) {
    auto* p = m.find( static_cast<uint8_t>( k ) );
    if ( has[k] ) {
      ++n;
      CHECK( p );
      if ( p ) {
        CHECK( p->v() == ref[k] );
        if constexpr ( N > 4 ) CHECK( p->b[N - 1] == static_cast<unsigned char>( ref[k] ) );
      }
    } else {
      CHECK( !p );
    }
    CHECK( m.has( static_cast<uint8_t>( k ) ) == has[k] );
  }
  CHECK( m.size() == n );
  CHECK( m.capacity() >= m.size() );
  // Iteration order = key order, values dense.
  size_t c = 0;
  int last = -1;
  for ( auto [k, val] : m ) {
    CHECK( static_cast<int>( k ) > last );
    last = k;
    CHECK( has[k] && val.v() == ref[k] );
    CHECK( &val == m.values() + c );
    ++c;
  }
  CHECK( c == n );
}

template <size_t N>
static void randomOps( int t ) {
  map<Val<N>> m = ( t & 1 ) ? map<Val<N>>() : map<Val<N>>( static_cast<unsigned>( std::rand() % 40 ), GENERAL );
  bool has[256]{};
  unsigned ref[256]{};
  for ( int op = 0; op < 3000; ++op ) {
    const int k = std::rand() % 256;
    const int r = std::rand() % 10;
    if ( r < 6 ) {
      const unsigned v = static_cast<unsigned>( std::rand() );
      m.set( static_cast<uint8_t>( k ), Val<N>::make( v ) );
      has[k] = true;
      ref[k] = v;
    } else if ( r < 9 ) {
      CHECK( m.erase( static_cast<uint8_t>( k ) ) == has[k] );
      has[k] = false;
    } else if ( op % 97 == 0 ) {
      m.shrink_to_fit();
    }
    if ( op % 50 == 0 ) verify( m, has, ref );
  }
  verify( m, has, ref );
}

int main() {
  const size_t generalBefore = memStats( GENERAL ).inUse;
  {  // overwrite
    map<Val<16>> m;
    for ( unsigned v = 0; v < 5; ++v ) m.set( 10, Val<16>::make( v ) );
    m.set( 3, Val<16>::make( 77 ) );
    m.set( 10, Val<16>::make( 99 ) );
    CHECK( m.size() == 2 );
    CHECK( m.find( 10 )->v() == 99 );
    CHECK( m.find( 3 )->v() == 77 );
    std::puts( "ok overwrite" );
  }
  {  // key 255 and all 256 keys
    map<unsigned> m;
    m.set( 255, 255u );
    CHECK( *m.find( 255 ) == 255 );
    CHECK( !m.find( 254 ) );
    CHECK( m.capacity() == 4 );
    for ( unsigned v = 0; v < 256; ++v ) m.set( static_cast<uint8_t>( v ), v );
    CHECK( m.size() == 256 && m.capacity() == 256 );
    for ( unsigned v = 0; v < 256; ++v ) CHECK( *m.find( static_cast<uint8_t>( v ) ) == v );
    m.erase( 255 );
    CHECK( !m.find( 255 ) );
    CHECK( m.size() == 255 );
    std::puts( "ok key255/full256" );
  }
  {  // exact capacity, grow, shrink
    map<Val<64>> m( 8, GENERAL );
    for ( unsigned v = 0; v < 8; ++v ) m.set( static_cast<uint8_t>( v * 31 ), Val<64>::make( v ) );
    CHECK( m.capacity() == 8 );
    m.set( 1, Val<64>::make( 8 ) );
    CHECK( m.capacity() == 16 );
    m.shrink_to_fit();
    CHECK( m.capacity() == 9 );
    std::puts( "ok exact/grow/shrink" );
  }
  std::srand( 42 );
  for ( int t = 0; t < 300; ++t ) {
    switch ( t % 3 ) {
      case 0: randomOps<4>( t ); break;
      case 1: randomOps<16>( t ); break;
      default: randomOps<64>( t ); break;
    }
  }
  std::puts( "ok random ops vs reference (300 maps x 3000 ops, keys 0..255)" );
  {  // non-trivial values: shared_ptr / string survive shifts, erases, growth, copies and moves
    auto live = std::make_shared<int>( 7 );
    {
      map<std::shared_ptr<int>> m;
      for ( int k = 200; k >= 0; k -= 10 ) m.set( static_cast<uint8_t>( k ), live );  // descending: shifts
      CHECK( live.use_count() == 22 );
      m.erase( 100 );
      CHECK( live.use_count() == 21 );
      map<std::shared_ptr<int>> c( m );
      CHECK( live.use_count() == 41 );
      map<std::shared_ptr<int>> mv( std::move( c ) );
      CHECK( c.empty() && live.use_count() == 41 );
      m.clear();
      CHECK( live.use_count() == 21 );
    }
    CHECK( live.use_count() == 1 );
    map<std::string> s;
    s[5] = "five";
    s[1] = "one";
    s[9] = "nine";
    s.emplace( 3, "three" );
    std::string joined;
    for ( auto [k, v] : s ) joined += std::to_string( k ) + v;
    CHECK( joined == "1one3three5five9nine" );
    s.erase( 3 );
    CHECK( *s.find( 5 ) == "five" && s.size() == 3 );
    std::puts( "ok non-trivial values" );
  }
  {  // pools: values land in the chosen pool and go back to it
    map<uint32_t, GRAPHIC> g;
    g.set( 1, 1u );
    CHECK( memPoolOf( g.values() ) == GRAPHIC );
    map<uint32_t> a( AUDIO );
    a.set( 2, 2u );
    CHECK( memPoolOf( a.values() ) == AUDIO );
    const size_t before = memStats( SCENE ).inUse;
    {
      map<std::array<uint8_t, 100>> sc( SCENE );
      for ( int k = 0; k < 50; ++k ) sc[static_cast<uint8_t>( k )];
      CHECK( memStats( SCENE ).inUse > before );
    }
    CHECK( memStats( SCENE ).inUse == before );
    std::puts( "ok pools" );
  }
  CHECK( memStats( GENERAL ).inUse == generalBefore );  // nothing leaked
  if ( failures ) std::printf( "%d failures\n", failures );
  return failures ? 1 : 0;
}
