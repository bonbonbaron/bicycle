#include "data/Mesh.h"
#include "data/Edgebreaker.h"
#include "data/Pack.h"
#include "Bytes.h"
#include <algorithm>
#include <cmath>

namespace mesh {

void octDecode( int32_t a, int32_t b, float n[3] ) {
  const float maxv = static_cast<float>( ( 1 << ( NORMAL_BITS - 1 ) ) - 1 );
  float x = a / maxv, y = b / maxv;
  float z = 1 - std::fabs( x ) - std::fabs( y );
  if ( z < 0 ) {
    const float ox = x;
    x = ( 1 - std::fabs( y ) ) * ( ox >= 0 ? 1.f : -1.f );
    y = ( 1 - std::fabs( ox ) ) * ( y >= 0 ? 1.f : -1.f );
  }
  const float len = std::sqrt( x * x + y * y + z * z );
  n[0] = x / len; n[1] = y / len; n[2] = z / len;
}

auto predict( const std::vector<int32_t>& q, const std::vector<uint8_t>& known, std::array<int32_t, 3> ctx,
              int32_t last, int axis, int qbits ) -> int32_t {
  auto ok = [&]( int32_t i ) { return i >= 0 && known[i]; };
  auto at = [&]( int32_t i ) { return q[3 * i + axis]; };
  const auto [u, v, o] = ctx;
  int32_t p;
  if ( ok( u ) && ok( v ) && ok( o ) ) p = at( u ) + at( v ) - at( o );   // parallelogram
  else if ( ok( u ) && ok( v ) ) p = ( at( u ) + at( v ) ) / 2;
  else if ( ok( u ) ) p = at( u );
  else if ( ok( v ) ) p = at( v );
  else if ( last >= 0 ) p = at( last );
  else p = 0;
  return std::clamp( p, 0, ( 1 << qbits ) - 1 );
}

auto decode( std::string_view s ) -> std::optional<MeshData> {
  bytes::Reader r( s );
  if ( r.str( 4 ) != "BMSH" || r.u32() != 1 ) return std::nullopt;
  const uint32_t nVerts = r.u32(), nTris = r.u32();
  const uint8_t flags = r.u8();
  const int qbits = r.u8();
  float bmin[3], bmax[3], uvmin[2], uvmax[2];
  for ( auto& f : bmin ) f = r.f32();
  for ( auto& f : bmax ) f = r.f32();
  for ( auto& f : uvmin ) f = r.f32();
  for ( auto& f : uvmax ) f = r.f32();
  const uint32_t nDecoded = r.u32();
  MeshData m;
  m.materials.resize( r.u16() );
  for ( auto& mat : m.materials ) {
    mat.name = r.str( r.u16() );
    for ( auto& c : mat.rgba ) c = r.f32();
    mat.texture = r.str( r.u16() );
  }
  std::string streams[8];
  for ( int si = 0; si < ( flags & 8 ? 8 : 7 ); ++si ) {
    auto& st = streams[si];
    const uint32_t raw = r.u32(), comp = r.u32();
    if ( !r.need( comp ) ) return std::nullopt;
    auto inflated = assets::inflateZlib( reinterpret_cast<const uint8_t*>( s.data() + r.pos ), comp, raw );
    if ( !inflated ) return std::nullopt;
    st = std::move( *inflated );
    r.pos += comp;
  }
  if ( !r.ok || qbits < 1 || qbits > 24 ) return std::nullopt;
  // Connectivity.
  eb::Connectivity conn;
  conn.raw = flags & 1;
  conn.nVerts = nDecoded;
  conn.ops.assign( streams[0].begin(), streams[0].end() );
  {
    bytes::Reader o( streams[1] );
    while ( o.pos < streams[1].size() && o.ok ) conn.offsets.push_back( o.varint() );
    bytes::Reader t( streams[2] );
    uint32_t prev = 0;
    while ( t.pos < streams[2].size() && t.ok ) {
      prev = static_cast<uint32_t>( static_cast<int32_t>( prev ) + t.svarint() );
      conn.rawTris.push_back( prev );
    }
  }
  eb::Decoded d;
  if ( !eb::decode( conn, d ) ) return std::nullopt;
  // Positions in decoded order (dummies have none).
  std::vector<int32_t> q( 3 * static_cast<std::size_t>( d.nVerts ), 0 );
  std::vector<uint8_t> known( d.nVerts, 0 );
  std::vector<int32_t> compact( d.nVerts, -1 );
  bytes::Reader pr( streams[3] );
  int32_t last = -1;
  uint32_t nReal = 0;
  for ( uint32_t i = 0; i < d.nVerts; ++i ) {
    if ( d.dummy[i] ) continue;
    for ( int a = 0; a < 3; ++a ) {
      q[3 * i + a] = predict( q, known, d.pred[i], last, a, qbits ) + pr.svarint();
    }
    known[i] = 1;
    last = static_cast<int32_t>( i );
    compact[i] = static_cast<int32_t>( nReal++ );
  }
  if ( !pr.ok || nReal != nVerts ) return std::nullopt;
  m.pos.resize( 3 * static_cast<std::size_t>( nVerts ) );
  const float steps = static_cast<float>( ( 1 << qbits ) - 1 );
  for ( uint32_t i = 0; i < d.nVerts; ++i ) {
    if ( compact[i] < 0 ) continue;
    for ( int a = 0; a < 3; ++a ) {
      m.pos[3 * compact[i] + a] = bmin[a] + ( bmax[a] - bmin[a] ) * q[3 * i + a] / steps;
    }
  }
  // Real triangles in decoded order.
  for ( std::size_t t = 0; t < d.tris.size() / 3; ++t ) {
    const int32_t a = compact[d.tris[3 * t]], b = compact[d.tris[3 * t + 1]], c = compact[d.tris[3 * t + 2]];
    if ( a < 0 || b < 0 || c < 0 ) continue;
    m.tris.insert( m.tris.end(), { static_cast<uint32_t>( a ), static_cast<uint32_t>( b ), static_cast<uint32_t>( c ) } );
  }
  if ( m.tris.size() != 3 * static_cast<std::size_t>( nTris ) ) return std::nullopt;
  const std::size_t nCorners = m.tris.size();
  if ( flags & 2 ) {
    bytes::Reader nr( streams[4] );
    int32_t pa = 0, pb = 0;
    m.normals.resize( 3 * nCorners );
    for ( std::size_t c = 0; c < nCorners; ++c ) {
      pa += nr.svarint();
      pb += nr.svarint();
      octDecode( pa, pb, &m.normals[3 * c] );
    }
    if ( !nr.ok ) return std::nullopt;
  }
  if ( flags & 4 ) {
    bytes::Reader ur( streams[5] );
    int32_t pu = 0, pv = 0;
    m.uvs.resize( 2 * nCorners );
    for ( std::size_t c = 0; c < nCorners; ++c ) {
      pu += ur.svarint();
      pv += ur.svarint();
      m.uvs[2 * c] = uvmin[0] + ( uvmax[0] - uvmin[0] ) * pu / 65535.f;
      m.uvs[2 * c + 1] = uvmin[1] + ( uvmax[1] - uvmin[1] ) * pv / 65535.f;
    }
    if ( !ur.ok ) return std::nullopt;
  }
  if ( flags & 8 ) {
    if ( streams[7].size() != 8 * static_cast<std::size_t>( nVerts ) ) return std::nullopt;
    m.joints.resize( 4 * static_cast<std::size_t>( nVerts ) );
    m.weights.resize( 4 * static_cast<std::size_t>( nVerts ) );
    for ( std::size_t v = 0; v < nVerts; ++v ) {
      for ( int k = 0; k < 4; ++k ) m.joints[4 * v + k] = static_cast<uint8_t>( streams[7][8 * v + k] );
      for ( int k = 0; k < 4; ++k ) m.weights[4 * v + k] = static_cast<uint8_t>( streams[7][8 * v + 4 + k] );
    }
  }
  m.triMat.assign( streams[6].begin(), streams[6].end() );
  if ( m.triMat.size() != nTris ) m.triMat.assign( nTris, 0 );
  return m;
}

}  // namespace mesh
