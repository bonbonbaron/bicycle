// Pools: see include/data/Mem.h. Same shape as jollybean's mem.c (one arena, a MemPool per PoolId with an origin
// and a nextFree pointer, memAdd/memRst/memRewind/memReport), plus block headers so blocks can be freed.
#include "data/Mem.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef BICYCLE_MEM_PERMANENT_MB
#define BICYCLE_MEM_PERMANENT_MB 16
#endif
#ifndef BICYCLE_MEM_GENERAL_MB
#define BICYCLE_MEM_GENERAL_MB 128
#endif
#ifndef BICYCLE_MEM_TEXT_MB
#define BICYCLE_MEM_TEXT_MB 64
#endif
#ifndef BICYCLE_MEM_GRAPHIC_MB
#define BICYCLE_MEM_GRAPHIC_MB 96
#endif
#ifndef BICYCLE_MEM_AUDIO_MB
#define BICYCLE_MEM_AUDIO_MB 96
#endif
#ifndef BICYCLE_MEM_TEMPORARY_MB
#define BICYCLE_MEM_TEMPORARY_MB 64
#endif
#ifndef BICYCLE_MEM_SCENE_MB
#define BICYCLE_MEM_SCENE_MB 64
#endif

namespace mem {
namespace {

constexpr size_t ALIGN = 16;
constexpr size_t alignUp( size_t x, size_t a = ALIGN ) { return ( x + a - 1 ) & ~( a - 1 ); }

const char* POOL_NAMES[N_POOLS] = { "Permanent", "General", "Text", "Graphic", "Audio", "Temporary", "Scene" };
const char* POOL_ENV[N_POOLS] = { "BICYCLE_MEM_PERMANENT_MB", "BICYCLE_MEM_GENERAL_MB", "BICYCLE_MEM_TEXT_MB",
                                  "BICYCLE_MEM_GRAPHIC_MB", "BICYCLE_MEM_AUDIO_MB", "BICYCLE_MEM_TEMPORARY_MB",
                                  "BICYCLE_MEM_SCENE_MB" };
const size_t POOL_DEFAULT_MB[N_POOLS] = { BICYCLE_MEM_PERMANENT_MB, BICYCLE_MEM_GENERAL_MB, BICYCLE_MEM_TEXT_MB,
                                          BICYCLE_MEM_GRAPHIC_MB, BICYCLE_MEM_AUDIO_MB, BICYCLE_MEM_TEMPORARY_MB,
                                          BICYCLE_MEM_SCENE_MB };
thread_local bool tSceneScope = false;  // memSceneScope: this thread's GRAPHIC/AUDIO allocations go to SCENE

// Every block starts with a header; the caller's pointer follows it (16-byte aligned).
constexpr uint32_t MAGIC = 0xB1C7C1E5u;
constexpr uint8_t BIG = 0xFF;          // size class of a block too big for the classes (first-fit list)
constexpr int N_CLASSES = 13;          // 16 B .. 64 KB
constexpr size_t MAX_CLASS_SIZE = size_t( ALIGN ) << ( N_CLASSES - 1 );
struct Header {
  uint64_t size;   // payload bytes
  uint32_t magic;
  uint8_t pool;
  uint8_t cls;
  uint16_t epoch;  // pool resets so far; a block from before the last reset isn't freed again
};
static_assert( sizeof( Header ) == ALIGN, "header keeps payloads aligned" );
struct FreeNode { FreeNode* next; };

struct MemPool {
  size_t memAllocated{};        // capacity
  uint8_t* nextFreeP{};
  const uint8_t* originP{};
  uint8_t* lastP{};             // last allocation's header (memRewind)
  FreeNode* freeLists[N_CLASSES]{};
  FreeNode* bigFree{};
  uint16_t epoch{};
  size_t highWater{}, inUse{}, peakInUse{}, allocs{};
};

struct Mem {
  uint8_t* dataP{};
  MemPool poolA[N_POOLS];
  bool ready{};
};

Mem _mem;
std::atomic_flag _lock = ATOMIC_FLAG_INIT;
struct Lock {
  Lock() { while ( _lock.test_and_set( std::memory_order_acquire ) ) {} }
  ~Lock() { _lock.clear( std::memory_order_release ); }
};

void atExitReport() { memReport(); }

void init() {  // called under the lock, the first time anything allocates (maybe before main)
  size_t total = 0, sizes[N_POOLS];
  for ( int i = 0; i < N_POOLS; ++i ) {
    size_t mb = POOL_DEFAULT_MB[i];
    if ( const char* e = std::getenv( POOL_ENV[i] ) ) {
      if ( long v = std::atol( e ); v > 0 ) mb = static_cast<size_t>( v );
    }
    sizes[i] = mb << 20;
    total += sizes[i];
  }
  // One reservation for every pool, like jollybean's static _data[] (the OS commits pages as they're touched).
  _mem.dataP = static_cast<uint8_t*>( std::malloc( total ) );
  if ( !_mem.dataP ) {
    std::fprintf( stderr, "bicycle: can't reserve %zu MB for the memory pools\n", total >> 20 );
    std::abort();
  }
  uint8_t* p = _mem.dataP;
  for ( int i = 0; i < N_POOLS; ++i ) {
    auto& pool = _mem.poolA[i];
    pool.memAllocated = sizes[i];
    pool.originP = pool.nextFreeP = p;
    p += sizes[i];
  }
  _mem.ready = true;
  std::atexit( atExitReport );
}

auto classOf( size_t n ) -> uint8_t {
  if ( n > MAX_CLASS_SIZE ) return BIG;
  uint8_t c = 0;
  size_t s = ALIGN;
  while ( s < n ) { s <<= 1; ++c; }
  return c;
}

[[noreturn]] void overflow( PoolId id, size_t n ) {
  const auto& pool = _mem.poolA[id];
  std::fprintf( stderr, "bicycle: the %s memory pool is full (%zu MB): can't allocate %zu bytes. Raise %s.\n",
                POOL_NAMES[id], pool.memAllocated >> 20, n, POOL_ENV[id] );
  assert( !"memory pool overflow" );
  std::abort();
}

auto headerOf( const void* p ) -> Header* {
  return reinterpret_cast<Header*>( static_cast<uint8_t*>( const_cast<void*>( p ) ) - sizeof( Header ) );
}

auto addLocked( size_t numBytes, PoolId id ) -> void* {
  if ( !_mem.ready ) init();
  auto& pool = _mem.poolA[id];
  const uint8_t cls = classOf( numBytes );
  const size_t size = cls == BIG ? alignUp( numBytes, 256 ) : ( ALIGN << cls );
  Header* h = nullptr;
  if ( cls != BIG && pool.freeLists[cls] ) {  // reuse a freed block of this class
    FreeNode* n = pool.freeLists[cls];
    pool.freeLists[cls] = n->next;
    h = headerOf( n );
  }
  else if ( cls == BIG ) {  // first fit among freed big blocks
    for ( FreeNode** link = &pool.bigFree; *link; link = &( *link )->next ) {
      Header* cand = headerOf( *link );
      if ( cand->size >= size && cand->size <= 2 * size ) {
        *link = ( *link )->next;
        h = cand;
        break;
      }
    }
  }
  if ( !h ) {  // bump
    const size_t need = sizeof( Header ) + size;
    if ( static_cast<size_t>( pool.nextFreeP - pool.originP ) + need > pool.memAllocated ) overflow( id, numBytes );
    h = reinterpret_cast<Header*>( pool.nextFreeP );
    h->size = size;
    pool.lastP = pool.nextFreeP;
    pool.nextFreeP += need;
    pool.highWater = std::max( pool.highWater, static_cast<size_t>( pool.nextFreeP - pool.originP ) );
  }
  h->magic = MAGIC;
  h->pool = id;
  h->cls = cls;
  h->epoch = pool.epoch;
  pool.inUse += h->size;
  pool.peakInUse = std::max( pool.peakInUse, pool.inUse );
  ++pool.allocs;
  return h + 1;
}

void delLocked( void* p ) {
  if ( !p ) return;
  Header* h = headerOf( p );
  assert( h->magic == MAGIC && "memDel of a pointer that isn't from a pool" );
  if ( h->magic != MAGIC ) return;
  auto& pool = _mem.poolA[h->pool];
  if ( h->epoch != pool.epoch ) return;  // the pool was reset since: already free
  h->magic = 0;
  pool.inUse -= h->size;
  uint8_t* end = reinterpret_cast<uint8_t*>( h + 1 ) + h->size;
  if ( end == pool.nextFreeP ) {  // top of the pool: rewind instead
    pool.nextFreeP = reinterpret_cast<uint8_t*>( h );
    pool.lastP = nullptr;
    return;
  }
  auto* n = static_cast<FreeNode*>( p );
  if ( h->cls == BIG ) {
    n->next = pool.bigFree;
    pool.bigFree = n;
  }
  else {
    n->next = pool.freeLists[h->cls];
    pool.freeLists[h->cls] = n;
  }
}

}  // namespace

auto memAdd( size_t numBytes, PoolId poolId ) -> void* {
  if ( tSceneScope && ( poolId == GRAPHIC || poolId == AUDIO ) ) poolId = SCENE;
  Lock l;
  return addLocked( numBytes, poolId );
}

void memDel( void* p ) {
  Lock l;
  delLocked( p );
}

void memSceneScope( bool on ) {
  tSceneScope = on;
}

auto memInSceneScope() -> bool {
  return tSceneScope;
}

auto memSceneReset() -> size_t {
  size_t live{};
  {
    Lock l;
    if ( !_mem.ready ) return 0;
    live = _mem.poolA[SCENE].inUse;
  }
  if ( live == 0 ) memRst( SCENE );
  return live;
}

auto memResize( void* p, size_t numBytes, PoolId poolId ) -> void* {
  if ( !p ) return memAdd( numBytes, poolId );
  if ( numBytes == 0 ) {
    memDel( p );
    return nullptr;
  }
  Lock l;
  Header* h = headerOf( p );
  if ( h->size >= numBytes ) return p;  // still fits
  void* q = addLocked( numBytes, static_cast<PoolId>( h->pool ) );
  std::memcpy( q, p, h->size );
  delLocked( p );
  return q;
}

void memRst( PoolId poolId ) {
  Lock l;
  if ( !_mem.ready ) return;
  auto& pool = _mem.poolA[poolId];
  pool.nextFreeP = const_cast<uint8_t*>( pool.originP );
  pool.lastP = nullptr;
  for ( auto& f : pool.freeLists ) f = nullptr;
  pool.bigFree = nullptr;
  pool.inUse = 0;
  ++pool.epoch;
}

void memRstAll() {
  for ( int i = GENERAL; i < N_POOLS; ++i ) memRst( static_cast<PoolId>( i ) );
}

// Takes back the pool's last bump allocation (if it's still live).
void memRewind( PoolId poolId ) {
  Lock l;
  auto& pool = _mem.poolA[poolId];
  if ( !pool.lastP ) return;
  auto* h = reinterpret_cast<Header*>( pool.lastP );
  if ( h->magic == MAGIC ) pool.inUse -= h->size;
  h->magic = 0;
  pool.nextFreeP = pool.lastP;
  pool.lastP = nullptr;
}

auto memPoolOf( const void* p ) -> int {
  if ( !_mem.ready || !p ) return -1;
  for ( int i = 0; i < N_POOLS; ++i ) {
    const auto& pool = _mem.poolA[i];
    if ( p >= pool.originP && p < pool.originP + pool.memAllocated ) return i;
  }
  return -1;
}

auto memStats( PoolId id ) -> PoolStats {
  Lock l;
  const auto& pool = _mem.poolA[id];
  return { POOL_NAMES[id], pool.memAllocated, static_cast<size_t>( pool.nextFreeP - pool.originP ), pool.highWater,
           pool.inUse, pool.peakInUse, pool.allocs };
}

void memReport( bool force ) {
  if ( !force ) {
    const char* e = std::getenv( "BICYCLE_MEM_STATS" );
    if ( !e || !*e || *e == '0' ) return;
  }
  std::fprintf( stderr, "bicycle memory pools (high-water mark / capacity, peak live bytes, allocations):\n" );
  for ( int i = 0; i < N_POOLS; ++i ) {
    const auto s = memStats( static_cast<PoolId>( i ) );
    std::fprintf( stderr, "  %-9s %10zu / %10zu bytes (%5.1f%%)  peak live %10zu  allocs %8zu\n", s.name, s.highWater,
                  s.capacity, 100.0 * s.highWater / std::max<size_t>( 1, s.capacity ), s.peakInUse, s.allocs );
  }
}

auto Resource::do_allocate( size_t bytes, size_t align ) -> void* {
  assert( align <= ALIGN );
  (void)align;
  return memAdd( bytes, _pool );
}

auto resource( PoolId p ) -> Resource* {
  static Resource* rs[N_POOLS] = { new ( memAdd( sizeof( Resource ), PERMANENT ) ) Resource( PERMANENT ),
                                   new ( memAdd( sizeof( Resource ), PERMANENT ) ) Resource( GENERAL ),
                                   new ( memAdd( sizeof( Resource ), PERMANENT ) ) Resource( TEXT ),
                                   new ( memAdd( sizeof( Resource ), PERMANENT ) ) Resource( GRAPHIC ),
                                   new ( memAdd( sizeof( Resource ), PERMANENT ) ) Resource( AUDIO ),
                                   new ( memAdd( sizeof( Resource ), PERMANENT ) ) Resource( TEMPORARY ),
                                   new ( memAdd( sizeof( Resource ), PERMANENT ) ) Resource( SCENE ) };
  return rs[p];
}

}  // namespace mem
