// .bskin encoding (offline: bicycle-pack and the codec tests). The engine only decodes (src/data/Skin.cpp).
#include "codec/SkinEncode.h"
#include "../../src/data/Bytes.h"
#include <algorithm>
#include <cmath>

namespace skin {

namespace {

constexpr float SQRT2 = 1.41421356f;  // the same scale as quatDecode in src/data/Skin.cpp

auto clamp16( float v ) -> int32_t { return std::clamp( static_cast<int32_t>( std::lround( v ) ), -32767, 32767 ); }

// Smallest three: drop the largest component (its sign made positive), keep the other three.
void quatEncode( const float* q, uint8_t& idx, int32_t c[3] ) {
  int big = 0;
  for ( int i = 1; i < 4; ++i ) if ( std::fabs( q[i] ) > std::fabs( q[big] ) ) big = i;
  const float sign = q[big] < 0 ? -1.0f : 1.0f;
  idx = static_cast<uint8_t>( big );
  for ( int i = 0, k = 0; i < 4; ++i ) if ( i != big ) c[k++] = clamp16( q[i] * sign * SQRT2 * 32767.0f );
}

}  // namespace

auto encode( const Skin& k, bool quantized, bool withVertices ) -> std::string {
  std::string o = "BSKN";
  const bool verts = withVertices && !k.vJoints.empty();
  bytes::u8( o, 1 );
  bytes::u8( o, ( quantized ? 1 : 0 ) | ( verts ? 2 : 0 ) );
  bytes::u16( o, static_cast<uint16_t>( k.joints.size() ) );
  bytes::u16( o, static_cast<uint16_t>( k.actions.size() ) );
  bytes::u32( o, verts ? static_cast<uint32_t>( k.vJoints.size() / 4 ) : 0 );
  for ( const auto& j : k.joints ) {
    bytes::u8( o, static_cast<uint8_t>( j.name.size() ) ); o += j.name;
    bytes::u16( o, static_cast<uint16_t>( j.parent ) );
    for ( float f : j.inverseBind ) bytes::f32( o, f );
  }
  if ( verts ) {
    for ( size_t v = 0; v < k.vJoints.size() / 4; ++v ) {
      for ( int i = 0; i < 4; ++i ) bytes::u8( o, k.vJoints[4 * v + i] );
      for ( int i = 0; i < 4; ++i ) bytes::u8( o, k.vWeights[4 * v + i] );
    }
  }
  const size_t nJ = k.joints.size();
  for ( const auto& a : k.actions ) {
    bytes::u8( o, static_cast<uint8_t>( a.name.size() ) ); o += a.name;
    bytes::f32( o, a.fps );
    bytes::u16( o, static_cast<uint16_t>( a.nFrames ) );
    const size_t n = static_cast<size_t>( a.nFrames ) * nJ;
    if ( !quantized ) {
      for ( size_t i = 0; i < n; ++i ) {
        for ( int c = 0; c < 3; ++c ) bytes::f32( o, a.t[3 * i + c] );
        for ( int c = 0; c < 4; ++c ) bytes::f32( o, a.q[4 * i + c] );
      }
      continue;
    }
    float tScale = 1e-6f;
    for ( float v : a.t ) tScale = std::max( tScale, std::fabs( v ) );
    bytes::f32( o, tScale );
    std::vector<int32_t> prev( 7 * nJ, 0 );
    for ( size_t i = 0; i < n; ++i ) {
      int32_t cur[7];
      for ( int c = 0; c < 3; ++c ) cur[c] = clamp16( a.t[3 * i + c] / tScale * 32767.0f );
      uint8_t idx;
      quatEncode( &a.q[4 * i], idx, cur + 4 );
      cur[3] = idx;
      int32_t* p = &prev[7 * ( i % nJ )];
      for ( int c = 0; c < 7; ++c ) { bytes::svarint( o, cur[c] - p[c] ); p[c] = cur[c]; }
    }
  }
  return o;
}

}  // namespace skin
