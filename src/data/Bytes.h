#pragma once
// Little-endian byte reading/writing helpers for the asset formats.
#include <cstdint>
#include <cstring>
#include <string>

namespace bytes {

struct Reader {
  const std::string& s;
  size_t pos{};
  bool ok{true};
  explicit Reader( const std::string& str, size_t start = 0 ) : s( str ), pos( start ) {}
  auto need( size_t n ) -> bool {
    if ( !ok || pos + n > s.size() ) { ok = false; return false; }
    return true;
  }
  auto u8() -> uint8_t { if ( !need( 1 ) ) return 0; return static_cast<uint8_t>( s[pos++] ); }
  auto u16() -> uint16_t { uint16_t v = u8(); v |= static_cast<uint16_t>( u8() ) << 8; return v; }
  auto u32() -> uint32_t { uint32_t v = u16(); v |= static_cast<uint32_t>( u16() ) << 16; return v; }
  auto f32() -> float { uint32_t v = u32(); float f; std::memcpy( &f, &v, 4 ); return f; }
  auto str( size_t n ) -> std::string { if ( !need( n ) ) return {}; auto r = s.substr( pos, n ); pos += n; return r; }
  auto varint() -> uint32_t {
    uint32_t v{}; int shift{};
    while ( ok ) {
      const uint8_t b = u8();
      v |= static_cast<uint32_t>( b & 0x7f ) << shift;
      if ( !( b & 0x80 ) ) break;
      shift += 7;
      if ( shift > 28 ) { ok = false; break; }
    }
    return v;
  }
  auto svarint() -> int32_t { const uint32_t z = varint(); return static_cast<int32_t>( ( z >> 1 ) ^ ( ~( z & 1 ) + 1 ) ); }
};

inline void u8( std::string& o, uint8_t v ) { o.push_back( static_cast<char>( v ) ); }
inline void u16( std::string& o, uint16_t v ) { u8( o, v & 0xff ); u8( o, v >> 8 ); }
inline void u32( std::string& o, uint32_t v ) { u16( o, v & 0xffff ); u16( o, v >> 16 ); }
inline void f32( std::string& o, float f ) { uint32_t v; std::memcpy( &v, &f, 4 ); u32( o, v ); }
inline void varint( std::string& o, uint32_t v ) {
  while ( v >= 0x80 ) { u8( o, static_cast<uint8_t>( v | 0x80 ) ); v >>= 7; }
  u8( o, static_cast<uint8_t>( v ) );
}
inline void svarint( std::string& o, int32_t v ) {  // zigzag
  varint( o, ( static_cast<uint32_t>( v ) << 1 ) ^ static_cast<uint32_t>( v >> 31 ) );
}

}  // namespace bytes
