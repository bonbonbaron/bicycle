#include "data/Skin.h"
#include "Bytes.h"
#include <algorithm>
#include <cmath>

auto Skin::action( std::string_view name ) const -> const SkinAction* {
  for ( const auto& a : actions ) if ( a.name == name ) return &a;
  return nullptr;
}

namespace skin {

namespace {

constexpr float SQRT2 = 1.41421356f;

auto clamp16( float v ) -> int32_t { return std::clamp( static_cast<int32_t>( std::lround( v ) ), -32767, 32767 ); }

// Smallest three: drop the largest component (its sign made positive), keep the other three.
void quatEncode( const float* q, uint8_t& idx, int32_t c[3] ) {
  int big = 0;
  for ( int i = 1; i < 4; ++i ) if ( std::fabs( q[i] ) > std::fabs( q[big] ) ) big = i;
  const float sign = q[big] < 0 ? -1.0f : 1.0f;
  idx = static_cast<uint8_t>( big );
  for ( int i = 0, k = 0; i < 4; ++i ) if ( i != big ) c[k++] = clamp16( q[i] * sign * SQRT2 * 32767.0f );
}

void quatDecode( uint8_t idx, const int32_t c[3], float* q ) {
  float sum = 0;
  for ( int i = 0, k = 0; i < 4; ++i ) {
    if ( i == idx ) continue;
    q[i] = c[k++] / ( SQRT2 * 32767.0f );
    sum += q[i] * q[i];
  }
  q[idx & 3] = std::sqrt( std::max( 0.0f, 1.0f - sum ) );
}

using Mat = std::array<float, 16>;

auto mul( const Mat& a, const Mat& b ) -> Mat {  // column major
  Mat r{};
  for ( int c = 0; c < 4; ++c )
    for ( int rr = 0; rr < 4; ++rr ) {
      float s = 0;
      for ( int k = 0; k < 4; ++k ) s += a[k * 4 + rr] * b[c * 4 + k];
      r[c * 4 + rr] = s;
    }
  return r;
}

auto trs( const float* t, const float* q ) -> Mat {
  const float x = q[0], y = q[1], z = q[2], w = q[3];
  Mat m{};
  m[0] = 1 - 2 * ( y * y + z * z ); m[1] = 2 * ( x * y + z * w );     m[2] = 2 * ( x * z - y * w );
  m[4] = 2 * ( x * y - z * w );     m[5] = 1 - 2 * ( x * x + z * z ); m[6] = 2 * ( y * z + x * w );
  m[8] = 2 * ( x * z + y * w );     m[9] = 2 * ( y * z - x * w );     m[10] = 1 - 2 * ( x * x + y * y );
  m[12] = t[0]; m[13] = t[1]; m[14] = t[2]; m[15] = 1;
  return m;
}

}  // namespace

auto decode( std::string_view s ) -> std::optional<Skin> {
  bytes::Reader r( s );
  if ( r.str( 4 ) != "BSKN" || r.u8() != 1 ) return std::nullopt;
  const uint8_t flags = r.u8();
  const uint16_t nJoints = r.u16(), nActions = r.u16();
  const uint32_t nVerts = r.u32();
  if ( nJoints == 0 || nJoints > MAX_JOINTS ) return std::nullopt;
  Skin k;
  k.joints.resize( nJoints );
  for ( auto& j : k.joints ) {
    j.name = r.str( r.u8() );
    j.parent = static_cast<int16_t>( r.u16() );
    for ( auto& f : j.inverseBind ) f = r.f32();
  }
  for ( size_t i = 0; i < k.joints.size(); ++i ) {
    if ( k.joints[i].parent >= static_cast<int>( i ) ) return std::nullopt;  // parents first
  }
  if ( flags & 2 ) {
    if ( !r.need( 8 * static_cast<size_t>( nVerts ) ) ) return std::nullopt;
    k.vJoints.resize( 4 * static_cast<size_t>( nVerts ) );
    k.vWeights.resize( 4 * static_cast<size_t>( nVerts ) );
    for ( uint32_t v = 0; v < nVerts; ++v ) {
      for ( int i = 0; i < 4; ++i ) k.vJoints[4 * v + i] = r.u8();
      for ( int i = 0; i < 4; ++i ) k.vWeights[4 * v + i] = r.u8();
    }
  }
  k.actions.resize( nActions );
  for ( auto& a : k.actions ) {
    a.name = r.str( r.u8() );
    a.fps = r.f32();
    a.nFrames = r.u16();
    const size_t n = static_cast<size_t>( a.nFrames ) * nJoints;
    a.t.resize( 3 * n );
    a.q.resize( 4 * n );
    if ( !( flags & 1 ) ) {
      for ( size_t i = 0; i < n; ++i ) {
        for ( int c = 0; c < 3; ++c ) a.t[3 * i + c] = r.f32();
        for ( int c = 0; c < 4; ++c ) a.q[4 * i + c] = r.f32();
      }
    }
    else {
      const float tScale = r.f32();
      std::vector<int32_t> prev( 7 * static_cast<size_t>( nJoints ), 0 );
      for ( size_t i = 0; i < n; ++i ) {
        int32_t* p = &prev[7 * ( i % nJoints )];
        for ( int c = 0; c < 7; ++c ) p[c] += r.svarint();
        for ( int c = 0; c < 3; ++c ) a.t[3 * i + c] = p[c] / 32767.0f * tScale;
        quatDecode( static_cast<uint8_t>( p[3] ), p + 4, &a.q[4 * i] );
      }
    }
  }
  if ( !r.ok ) return std::nullopt;
  return k;
}

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

void restPose( const Skin& k, float* out ) {
  for ( size_t j = 0; j < k.joints.size(); ++j ) {
    for ( int i = 0; i < 16; ++i ) out[16 * j + i] = ( i % 5 == 0 ) ? 1.0f : 0.0f;
  }
}

void pose( const Skin& k, const SkinAction& a, float seconds, float* out ) {
  const size_t nJ = k.joints.size();
  if ( a.nFrames == 0 ) { restPose( k, out ); return; }
  const float f = std::fmod( std::max( 0.0f, seconds ) * a.fps, static_cast<float>( a.nFrames ) );
  const uint32_t f0 = static_cast<uint32_t>( f ) % a.nFrames, f1 = ( f0 + 1 ) % a.nFrames;
  const float u = f - std::floor( f );
  std::vector<Mat> global( nJ );
  for ( size_t j = 0; j < nJ; ++j ) {
    const size_t i0 = f0 * nJ + j, i1 = f1 * nJ + j;
    float t[3], q[4];
    for ( int c = 0; c < 3; ++c ) t[c] = a.t[3 * i0 + c] + ( a.t[3 * i1 + c] - a.t[3 * i0 + c] ) * u;
    const float dot = a.q[4 * i0] * a.q[4 * i1] + a.q[4 * i0 + 1] * a.q[4 * i1 + 1] + a.q[4 * i0 + 2] * a.q[4 * i1 + 2] + a.q[4 * i0 + 3] * a.q[4 * i1 + 3];
    const float s1 = dot < 0 ? -u : u;  // shortest way round
    float len = 0;
    for ( int c = 0; c < 4; ++c ) { q[c] = a.q[4 * i0 + c] * ( 1 - u ) + a.q[4 * i1 + c] * s1; len += q[c] * q[c]; }
    len = std::sqrt( std::max( len, 1e-12f ) );
    for ( auto& c : q ) c /= len;
    const Mat local = trs( t, q );
    global[j] = k.joints[j].parent < 0 ? local : mul( global[k.joints[j].parent], local );
    const Mat m = mul( global[j], k.joints[j].inverseBind );
    std::copy( m.begin(), m.end(), out + 16 * j );
  }
}

}  // namespace skin
