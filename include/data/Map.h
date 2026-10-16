#pragma once
// bicycle::map: a sparse map from small integer keys (uint8_t, 0..255) to values, ported from jollybean's
// src/data/map.c by way of /workspace/mapbench/map2.c (README, "Small maps"). The keys are build-time ids
// (gen/ids.h: animation tags, scenes, spawns, songs, colors, ...), so there's nothing to hash:
//
//   bits[4]   which of the 256 keys are present (4 x 64-bit words)
//   base[4]   how many keys are in the words before each word (prefix popcount)
//   vals      the values, densely, in key order: key k lives at base[k/64] + popcount(bits[k/64] below k)
//
// Lookups are a bit test and one popcount. Inserting in ascending key order appends (no shifting); inserting
// an existing key overwrites it in place. Value storage comes from a bump-allocator pool (data/Mem.h), chosen by
// the template parameter or the constructor like mem::Vector<T, POOL>; it starts empty and doubles (4, 8, ...,
// 256) unless constructed with an exact capacity. Iteration is in key order.
//
// Define BICYCLE_MAP_POPCNT_LUT for a byte-table popcount (cores without a popcount instruction).
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>
#include "data/Mem.h"

namespace bicycle {

namespace detail {
#ifdef BICYCLE_MAP_POPCNT_LUT
#define BICYCLE_B2( n ) n, n + 1, n + 1, n + 2
#define BICYCLE_B4( n ) BICYCLE_B2( n ), BICYCLE_B2( n + 1 ), BICYCLE_B2( n + 1 ), BICYCLE_B2( n + 2 )
#define BICYCLE_B6( n ) BICYCLE_B4( n ), BICYCLE_B4( n + 1 ), BICYCLE_B4( n + 1 ), BICYCLE_B4( n + 2 )
inline constexpr uint8_t POP8[256] = { BICYCLE_B6( 0 ), BICYCLE_B6( 1 ), BICYCLE_B6( 1 ), BICYCLE_B6( 2 ) };
#undef BICYCLE_B2
#undef BICYCLE_B4
#undef BICYCLE_B6
inline auto popcnt64( uint64_t x ) -> unsigned {
  unsigned c = 0;
  while ( x ) {  // early exit: the word is masked below the key
    c += POP8[x & 0xff];
    x >>= 8;
  }
  return c;
}
#else
inline auto popcnt64( uint64_t x ) -> unsigned { return static_cast<unsigned>( __builtin_popcountll( x ) ); }
#endif
}  // namespace detail

template <typename V, mem::PoolId DefaultPool = mem::GENERAL>
class map {
  public:
    using key_type = uint8_t;
    using mapped_type = V;
    static constexpr unsigned MAX = 256;

    explicit map( mem::PoolId pool = DefaultPool ) noexcept : _pool( pool ) {}
    // Exactly cap slots up front (still grows past them).
    map( unsigned exactCap, mem::PoolId pool ) : _pool( pool ) { reserveExact( exactCap ); }
    ~map() { release(); }

    map( const map& o ) : _pool( o._pool ) {
      reserveExact( o._n );
      for ( unsigned i = 0; i < o._n; ++i ) new ( _vals + i ) V( o._vals[i] );
      copyIndex( o );
    }
    map( map&& o ) noexcept : _pool( o._pool ) { steal( o ); }
    auto operator=( const map& o ) -> map& {
      if ( this != &o ) {
        map tmp( o );
        swap( tmp );
      }
      return *this;
    }
    auto operator=( map&& o ) noexcept -> map& {
      if ( this != &o ) {
        release();
        _pool = o._pool;
        steal( o );
      }
      return *this;
    }
    void swap( map& o ) noexcept {
      std::swap( _bits, o._bits );
      std::swap( _base, o._base );
      std::swap( _n, o._n );
      std::swap( _cap, o._cap );
      std::swap( _pool, o._pool );
      std::swap( _vals, o._vals );
    }

    auto has( key_type k ) const -> bool { return ( _bits[k >> 6] >> ( k & 63 ) ) & 1; }
    auto contains( key_type k ) const -> bool { return has( k ); }
    auto count( key_type k ) const -> size_t { return has( k ) ? 1 : 0; }
    // The value for k, or nullptr.
    auto find( key_type k ) -> V* {
      const unsigned w = k >> 6, b = k & 63;
      if ( !( ( _bits[w] >> b ) & 1 ) ) return nullptr;
      return _vals + idx( w, b );
    }
    auto find( key_type k ) const -> const V* { return const_cast<map*>( this )->find( k ); }

    // Inserts or overwrites; returns the stored value.
    template <typename U>
    auto set( key_type k, U&& v ) -> V& {
      if ( V* p = find( k ) ) {
        *p = std::forward<U>( v );
        return *p;
      }
      return *new ( makeSlot( k ) ) V( std::forward<U>( v ) );
    }
    template <typename... Args>
    auto emplace( key_type k, Args&&... args ) -> V& {
      if ( V* p = find( k ) ) {
        *p = V( std::forward<Args>( args )... );
        return *p;
      }
      return *new ( makeSlot( k ) ) V( std::forward<Args>( args )... );
    }
    // The value for k, default-constructed if it wasn't there.
    auto operator[]( key_type k ) -> V& {
      if ( V* p = find( k ) ) return *p;
      return *new ( makeSlot( k ) ) V();
    }

    // Returns true if k was there.
    auto erase( key_type k ) -> bool {
      const unsigned w = k >> 6, b = k & 63;
      if ( !( ( _bits[w] >> b ) & 1 ) ) return false;
      const unsigned i = idx( w, b );
      if constexpr ( std::is_trivially_copyable_v<V> ) {
        std::memmove( _vals + i, _vals + i + 1, static_cast<size_t>( _n - i - 1 ) * sizeof( V ) );
      } else {
        for ( unsigned j = i; j + 1 < _n; ++j ) _vals[j] = std::move( _vals[j + 1] );
        _vals[_n - 1].~V();
      }
      _bits[w] &= ~( 1ull << b );
      for ( unsigned j = w + 1; j < 4; ++j ) --_base[j];
      --_n;
      return true;
    }
    void clear() {
      destroyAll();
      _bits[0] = _bits[1] = _bits[2] = _bits[3] = 0;
      _base[0] = _base[1] = _base[2] = _base[3] = 0;
      _n = 0;
    }
    void shrink_to_fit() {
      if ( _n == _cap ) return;
      if ( !_n ) {
        mem::memDel( _vals );
        _vals = nullptr;
        _cap = 0;
        return;
      }
      relocate( _n );
    }
    void reserve( unsigned cap ) {
      if ( cap > _cap ) relocate( cap > MAX ? MAX : cap );
    }

    auto size() const -> size_t { return _n; }
    auto empty() const -> bool { return _n == 0; }
    auto capacity() const -> size_t { return _cap; }
    auto pool() const -> mem::PoolId { return _pool; }
    // Header plus value storage (what the benchmarks call bytes).
    auto bytes() const -> size_t { return sizeof( *this ) + static_cast<size_t>( _cap ) * sizeof( V ); }
    auto values() -> V* { return _vals; }  // dense, in key order
    auto values() const -> const V* { return _vals; }

    // Iteration in key order: for ( auto [key, value] : m ).
    template <typename M, typename R>
    struct iter {
      M* m;
      unsigned w;
      uint64_t rest;  // keys of word w not visited yet
      unsigned i;     // value index
      struct item {
        key_type first;
        R& second;
      };
      auto operator*() const -> item {
        return { static_cast<key_type>( w * 64 + __builtin_ctzll( rest ) ), m->_vals[i] };
      }
      auto operator++() -> iter& {
        rest &= rest - 1;
        ++i;
        skip();
        return *this;
      }
      void skip() {
        while ( !rest && w < 4 ) {
          if ( ++w < 4 ) rest = m->_bits[w];
        }
      }
      auto operator!=( const iter& o ) const -> bool { return i != o.i; }
      auto operator==( const iter& o ) const -> bool { return i == o.i; }
    };
    using iterator = iter<map, V>;
    using const_iterator = iter<const map, const V>;
    auto begin() -> iterator { return start<iterator>( this ); }
    auto end() -> iterator { return { this, 4, 0, _n }; }
    auto begin() const -> const_iterator { return start<const_iterator>( this ); }
    auto end() const -> const_iterator { return { this, 4, 0, _n }; }

  private:
    uint64_t _bits[4]{};
    uint16_t _base[4]{};
    uint16_t _n{};
    uint16_t _cap{};
    mem::PoolId _pool;
    V* _vals{};

    template <typename It, typename M>
    static auto start( M* m ) -> It {
      It it{ m, 0, m->_bits[0], 0 };
      it.skip();
      return it;
    }
    auto idx( unsigned w, unsigned b ) const -> unsigned {
      return _base[w] + detail::popcnt64( _bits[w] & ( ( 1ull << b ) - 1 ) );  // b < 64: no UB
    }
    void reserveExact( unsigned cap ) {
      if ( cap > MAX ) cap = MAX;
      if ( cap ) relocate( cap );
    }
    // Moves the values into storage for cap of them.
    void relocate( unsigned cap ) {
      V* nv = static_cast<V*>( mem::memAdd( static_cast<size_t>( cap ) * sizeof( V ), _pool ) );
      if ( _vals ) {
        if constexpr ( std::is_trivially_copyable_v<V> ) {
          std::memcpy( static_cast<void*>( nv ), _vals, static_cast<size_t>( _n ) * sizeof( V ) );
        } else {
          for ( unsigned i = 0; i < _n; ++i ) {
            new ( nv + i ) V( std::move( _vals[i] ) );
            _vals[i].~V();
          }
        }
        mem::memDel( _vals );
      }
      _vals = nv;
      _cap = static_cast<uint16_t>( cap );
    }
    // Room for a new key k (not present); returns its uninitialized slot.
    auto makeSlot( key_type k ) -> void* {
      const unsigned w = k >> 6, b = k & 63;
      const unsigned i = idx( w, b );
      if ( _n == _cap ) relocate( _cap ? ( _cap * 2u > MAX ? MAX : _cap * 2u ) : 4u );
      if ( __builtin_expect( i < _n, 0 ) ) {  // not an append: shift the later values up by one
        if constexpr ( std::is_trivially_copyable_v<V> ) {
          std::memmove( static_cast<void*>( _vals + i + 1 ), _vals + i, static_cast<size_t>( _n - i ) * sizeof( V ) );
        } else {
          new ( _vals + _n ) V( std::move( _vals[_n - 1] ) );
          for ( unsigned j = _n - 1; j > i; --j ) _vals[j] = std::move( _vals[j - 1] );
          _vals[i].~V();
        }
      }
      _bits[w] |= 1ull << b;
      for ( unsigned j = w + 1; j < 4; ++j ) ++_base[j];
      ++_n;
      return _vals + i;
    }
    void destroyAll() {
      if constexpr ( !std::is_trivially_destructible_v<V> ) {
        for ( unsigned i = 0; i < _n; ++i ) _vals[i].~V();
      }
    }
    void release() {
      destroyAll();
      if ( _vals ) mem::memDel( _vals );
      _vals = nullptr;
      _n = _cap = 0;
    }
    void copyIndex( const map& o ) {
      for ( int j = 0; j < 4; ++j ) {
        _bits[j] = o._bits[j];
        _base[j] = o._base[j];
      }
      _n = o._n;
    }
    void steal( map& o ) noexcept {
      copyIndex( o );
      _cap = o._cap;
      _vals = o._vals;
      o._vals = nullptr;
      o._n = o._cap = 0;
      for ( int j = 0; j < 4; ++j ) o._bits[j] = o._base[j] = 0;
    }
};

}  // namespace bicycle
