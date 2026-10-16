#pragma once
// bicycle's memory: bump-allocated pools, ported from jollybean (src/data/mem.c, include/data/mem.h).
//
// One arena is reserved at start-up and cut into six pools, each a bump allocator: memAdd() hands out the next
// aligned bytes and moves nextFree forward; memRst() moves it back to the pool's origin, freeing everything at
// once; memRewind() takes back the last allocation. On top of jollybean's design, freed blocks are kept on
// per-pool free lists (power-of-two size classes for small blocks, first fit for big ones) so long-running
// churn (std::string, std::vector growth, the Lua heap) reuses memory instead of exhausting the pool, and a
// block freed at the top of its pool rewinds it.
//
//   PERMANENT  lives for the whole run (the asset pack's bytes and index, engine singletons)
//   GENERAL    everything else, including every global operator new in the engine (std containers etc.)
//   TEXT       Lua: the LuaJIT heap, scripts, genomes, Aseprite exports
//   GRAPHIC    images, meshes, textures, the pixel framebuffer, GL upload staging
//   AUDIO      SoundFonts and MIDI files
//   TEMPORARY  scratch (decoding); reset every frame and after every asset load: never keep a pointer to it
//
// Sizes: BICYCLE_MEM_<POOL>_MB (CMake cache variable, default below) or the same environment variable at run
// time. Running out of a pool is fatal (it prints which pool and its size). BICYCLE_MEM_STATS=1 prints each
// pool's high-water mark at exit.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <new>
#include <string>
#include <string_view>
#include <vector>

namespace mem {

enum PoolId : uint8_t { PERMANENT, GENERAL, TEXT, GRAPHIC, AUDIO, TEMPORARY, N_POOLS };

// jollybean's API
auto memAdd( size_t numBytes, PoolId poolId ) -> void*;  // aligned to 16 bytes; never null (overflow is fatal)
void memDel( void* p );                                  // back to its pool's free list (or rewinds the pool)
auto memResize( void* p, size_t numBytes, PoolId poolId ) -> void*;  // realloc semantics (p may be null)
void memRst( PoolId poolId );   // frees the whole pool
void memRstAll();               // every pool but PERMANENT
void memRewind( PoolId poolId );  // takes back the pool's last allocation
void memReport( bool force = false );  // high-water marks (printed at exit when BICYCLE_MEM_STATS=1)
auto memPoolOf( const void* p ) -> int;  // the pool p was allocated from, or -1

struct PoolStats {
  const char* name;
  size_t capacity;   // bytes
  size_t bumped;     // current bump offset
  size_t highWater;  // highest bump offset ever
  size_t inUse;      // bytes in live blocks
  size_t peakInUse;
  size_t allocs;
};
auto memStats( PoolId poolId ) -> PoolStats;

// A std allocator over a pool. The pool is a template default but stored per instance, so one container type
// can live in any pool: mem::Vector<float> v( mem::Allocator<float>( mem::GRAPHIC ) ).
template <typename T, PoolId DefaultPool = GENERAL>
struct Allocator {
  using value_type = T;
  PoolId pool{DefaultPool};
  Allocator() noexcept = default;
  explicit Allocator( PoolId p ) noexcept : pool( p ) {}
  template <typename U, PoolId P>
  Allocator( const Allocator<U, P>& o ) noexcept : pool( o.pool ) {}
  template <typename U>
  struct rebind { using other = Allocator<U, DefaultPool>; };
  auto allocate( size_t n ) -> T* {
    if ( n > std::numeric_limits<size_t>::max() / sizeof( T ) ) throw std::bad_array_new_length();
    return static_cast<T*>( memAdd( n * sizeof( T ), pool ) );
  }
  void deallocate( T* p, size_t ) noexcept { memDel( p ); }
  template <typename U, PoolId P>
  auto operator==( const Allocator<U, P>& o ) const noexcept -> bool { return pool == o.pool; }
  template <typename U, PoolId P>
  auto operator!=( const Allocator<U, P>& o ) const noexcept -> bool { return pool != o.pool; }
};

template <typename T, PoolId P = GENERAL>
using Vector = std::vector<T, Allocator<T, P>>;
template <PoolId P = GENERAL>
using String = std::basic_string<char, std::char_traits<char>, Allocator<char, P>>;
// A loaded asset's bytes, in whichever pool it was loaded into.
using Blob = std::basic_string<char, std::char_traits<char>, Allocator<char, GENERAL>>;

// The same pools as std::pmr memory resources, for pmr containers.
class Resource final : public std::pmr::memory_resource {
  public:
    explicit Resource( PoolId p ) : _pool( p ) {}
  private:
    PoolId _pool;
    auto do_allocate( size_t bytes, size_t align ) -> void* override;
    void do_deallocate( void* p, size_t, size_t ) override { memDel( p ); }
    auto do_is_equal( const std::pmr::memory_resource& o ) const noexcept -> bool override { return this == &o; }
};
auto resource( PoolId p ) -> Resource*;

// Resets TEMPORARY when it goes out of scope (wrap an asset load or a frame in one).
struct TempScope {
  ~TempScope() { memRst( TEMPORARY ); }
};

}  // namespace mem
