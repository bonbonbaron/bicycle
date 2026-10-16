// The SCENE pool (include/data/Mem.h): while a SceneScope is open GRAPHIC/AUDIO allocations land in SCENE, and
// memSceneReset() only rewinds the pool once everything in it was freed. `ctest -R mem-scene`.
#include "data/Mem.h"
#include <cstdio>
using namespace mem;

static int failures = 0;
#define CHECK( cond, ... ) do { if ( !( cond ) ) { ++failures; std::printf( "FAIL %s:%d: ", __FILE__, __LINE__ ); std::printf( __VA_ARGS__ ); std::printf( "\n" ); } } while ( 0 )

int main() {
  // Outside a scope: no redirection.
  void* g = memAdd( 1000, GRAPHIC );
  CHECK( memPoolOf( g ) == GRAPHIC, "GRAPHIC outside a scope went to pool %d", memPoolOf( g ) );
  size_t hw = 0;
  for ( int cycle = 0; cycle < 50; ++cycle ) {
    void *a, *b, *c;
    {
      SceneScope scope;
      CHECK( memInSceneScope(), "scope not open" );
      a = memAdd( 4096, GRAPHIC );
      b = memAdd( 777, AUDIO );
      c = memAdd( 64, GENERAL );  // GENERAL is never redirected (persistent containers grow there)
    }
    CHECK( !memInSceneScope(), "scope still open" );
    CHECK( memPoolOf( a ) == SCENE && memPoolOf( b ) == SCENE, "scene loads went to %d/%d", memPoolOf( a ), memPoolOf( b ) );
    CHECK( memPoolOf( c ) == GENERAL, "GENERAL redirected to %d", memPoolOf( c ) );
    memDel( c );
    // Something still alive: the pool must not be rewound under it.
    memDel( a );
    size_t leaked = memSceneReset();
    CHECK( leaked > 0, "reset with a live block reported 0 leaked" );
    CHECK( memStats( SCENE ).inUse > 0, "live block lost" );
    memDel( b );
    CHECK( memSceneReset() == 0, "reset after freeing everything still leaks" );
    CHECK( memStats( SCENE ).bumped == 0, "pool not rewound (bumped %zu)", memStats( SCENE ).bumped );
    if ( cycle == 0 ) hw = memStats( SCENE ).highWater;
    CHECK( memStats( SCENE ).highWater == hw, "high-water grew to %zu (cycle %d, first %zu)", memStats( SCENE ).highWater, cycle, hw );
  }
  memDel( g );
  if ( failures ) { std::printf( "%d failure(s)\n", failures ); return 1; }
  std::printf( "mem scene: OK (SCENE high-water %zu bytes over 50 cycles)\n", hw );
  return 0;
}
