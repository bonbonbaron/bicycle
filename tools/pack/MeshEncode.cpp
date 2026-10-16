#include "MeshEncode.h"
#include "Deflate.h"
#include "../../src/data/Bytes.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>

namespace {
auto dirOf( const std::string& p ) -> std::string {
  const auto s = p.find_last_of( "/\\" );
  return s == std::string::npos ? "" : p.substr( 0, s + 1 );
}

auto readMtl( const std::string& path, std::map<std::string, MeshMaterial>& mats ) -> void {
  std::ifstream f( path );
  std::string line;
  MeshMaterial* cur = nullptr;
  while ( std::getline( f, line ) ) {
    std::istringstream ss( line );
    std::string tag;
    ss >> tag;
    if ( tag == "newmtl" ) {
      std::string name;
      std::getline( ss >> std::ws, name );
      cur = &mats[name];
      cur->name = name;
    }
    else if ( cur && tag == "Kd" ) ss >> cur->rgba[0] >> cur->rgba[1] >> cur->rgba[2];
    else if ( cur && tag == "d" ) ss >> cur->rgba[3];
    else if ( cur && tag == "map_Kd" ) {
      std::string rest;
      std::getline( ss >> std::ws, rest );
      // Options like "-s 1 1 1" may precede the file; the file name is the last token.
      const auto sp = rest.find_last_of( ' ' );
      cur->texture = sp == std::string::npos ? rest : rest.substr( sp + 1 );
    }
  }
}
}  // namespace

auto readObj( const std::string& objPath, MeshData& out, std::string& err ) -> bool {
  std::ifstream f( objPath );
  if ( !f ) { err = "can't open " + objPath; return false; }
  std::vector<float> v, vt, vn;
  std::map<std::string, MeshMaterial> mtlLib;
  std::map<std::string, uint8_t> matIdx;
  uint8_t curMat = 0;
  bool anyUv = false, anyNormal = false;
  struct Corner { int v, t, n; };
  std::vector<Corner> corners;
  std::vector<uint8_t> triMat;
  std::string line;
  while ( std::getline( f, line ) ) {
    std::istringstream ss( line );
    std::string tag;
    ss >> tag;
    if ( tag == "v" ) { float x, y, z; ss >> x >> y >> z; v.insert( v.end(), { x, y, z } ); }
    else if ( tag == "vt" ) { float a, b; ss >> a >> b; vt.insert( vt.end(), { a, b } ); }
    else if ( tag == "vn" ) { float x, y, z; ss >> x >> y >> z; vn.insert( vn.end(), { x, y, z } ); }
    else if ( tag == "mtllib" ) { std::string name; std::getline( ss >> std::ws, name ); readMtl( dirOf( objPath ) + name, mtlLib ); }
    else if ( tag == "usemtl" ) {
      std::string name;
      std::getline( ss >> std::ws, name );
      auto it = matIdx.find( name );
      if ( it == matIdx.end() ) {
        it = matIdx.emplace( name, static_cast<uint8_t>( out.materials.size() ) ).first;
        MeshMaterial m = mtlLib.count( name ) ? mtlLib[name] : MeshMaterial{};
        m.name = name;
        out.materials.push_back( m );
      }
      curMat = it->second;
    }
    else if ( tag == "f" ) {
      std::vector<Corner> poly;
      std::string tok;
      while ( ss >> tok ) {
        Corner c{ -1, -1, -1 };
        int* fields[3] = { &c.v, &c.t, &c.n };
        std::stringstream ts( tok );
        std::string part;
        for ( int i = 0; i < 3 && std::getline( ts, part, '/' ); ++i ) {
          if ( !part.empty() ) {
            int idx = std::stoi( part );
            const int count = i == 0 ? static_cast<int>( v.size() / 3 ) : i == 1 ? static_cast<int>( vt.size() / 2 ) : static_cast<int>( vn.size() / 3 );
            *fields[i] = idx < 0 ? count + idx : idx - 1;
          }
        }
        anyUv |= c.t >= 0;
        anyNormal |= c.n >= 0;
        poly.push_back( c );
      }
      for ( size_t i = 2; i < poly.size(); ++i ) {  // fan-triangulate
        corners.insert( corners.end(), { poly[0], poly[i - 1], poly[i] } );
        triMat.push_back( curMat );
      }
    }
  }
  if ( corners.empty() ) { err = objPath + " has no faces"; return false; }
  if ( out.materials.empty() ) out.materials.push_back( MeshMaterial{ "default" } );
  // Keep only referenced vertices.
  std::vector<int32_t> remap( v.size() / 3, -1 );
  for ( auto& c : corners ) {
    if ( c.v < 0 || c.v >= static_cast<int>( remap.size() ) ) { err = "bad vertex index"; return false; }
    if ( remap[c.v] < 0 ) {
      remap[c.v] = static_cast<int32_t>( out.pos.size() / 3 );
      out.pos.insert( out.pos.end(), { v[3 * c.v], v[3 * c.v + 1], v[3 * c.v + 2] } );
    }
    out.tris.push_back( remap[c.v] );
    if ( anyNormal ) {
      if ( c.n >= 0 ) out.normals.insert( out.normals.end(), { vn[3 * c.n], vn[3 * c.n + 1], vn[3 * c.n + 2] } );
      else out.normals.insert( out.normals.end(), { 0, 0, 1 } );
    }
    if ( anyUv ) {
      if ( c.t >= 0 ) out.uvs.insert( out.uvs.end(), { vt[2 * c.t], vt[2 * c.t + 1] } );
      else out.uvs.insert( out.uvs.end(), { 0, 0 } );
    }
  }
  out.triMat = triMat;
  return true;
}

static auto stream( std::string& o, const std::string& raw ) -> size_t {
  const std::string z = deflateZlib( raw );
  bytes::u32( o, raw.size() );
  bytes::u32( o, z.size() );
  o += z;
  return z.size() + 8;
}

auto meshEncode( const MeshData& m, int qbits, MeshEncodeInfo* info ) -> std::string {
  const uint32_t nVerts = static_cast<uint32_t>( m.nVerts() ), nTris = static_cast<uint32_t>( m.nTris() );
  eb::Encoded enc = eb::encode( m.tris, nVerts );
  eb::Decoded d;
  eb::decode( enc.conn, d );
  float bmin[3] = { 1e30f, 1e30f, 1e30f }, bmax[3] = { -1e30f, -1e30f, -1e30f };
  for ( uint32_t i = 0; i < nVerts; ++i ) {
    for ( int a = 0; a < 3; ++a ) {
      bmin[a] = std::min( bmin[a], m.pos[3 * i + a] );
      bmax[a] = std::max( bmax[a], m.pos[3 * i + a] );
    }
  }
  for ( int a = 0; a < 3; ++a ) if ( bmax[a] <= bmin[a] ) bmax[a] = bmin[a] + 1e-6f;
  float uvmin[2] = { 0, 0 }, uvmax[2] = { 1, 1 };
  if ( !m.uvs.empty() ) {
    uvmin[0] = uvmin[1] = 1e30f; uvmax[0] = uvmax[1] = -1e30f;
    for ( size_t c = 0; c < m.uvs.size() / 2; ++c ) {
      for ( int a = 0; a < 2; ++a ) {
        uvmin[a] = std::min( uvmin[a], m.uvs[2 * c + a] );
        uvmax[a] = std::max( uvmax[a], m.uvs[2 * c + a] );
      }
    }
    for ( int a = 0; a < 2; ++a ) if ( uvmax[a] <= uvmin[a] ) uvmax[a] = uvmin[a] + 1e-6f;
  }
  const float steps = static_cast<float>( ( 1 << qbits ) - 1 );
  // Quantized positions by decoded id.
  std::vector<int32_t> q( 3 * static_cast<size_t>( d.nVerts ), 0 );
  for ( uint32_t i = 0; i < d.nVerts; ++i ) {
    const int32_t o = enc.vertexOrigin[i];
    if ( o < 0 ) continue;
    for ( int a = 0; a < 3; ++a ) {
      q[3 * i + a] = static_cast<int32_t>( std::lround( ( m.pos[3 * o + a] - bmin[a] ) / ( bmax[a] - bmin[a] ) * steps ) );
    }
  }
  std::string ops( enc.conn.ops.begin(), enc.conn.ops.end() ), offs, rawTris, posS, nrmS, uvS, matS;
  for ( auto k : enc.conn.offsets ) bytes::varint( offs, k );
  if ( enc.conn.raw ) {
    uint32_t prev = 0;
    for ( auto t : enc.conn.rawTris ) { bytes::svarint( rawTris, static_cast<int32_t>( t - prev ) ); prev = t; }
  }
  std::vector<uint8_t> known( d.nVerts, 0 );
  int32_t last = -1;
  for ( uint32_t i = 0; i < d.nVerts; ++i ) {
    if ( d.dummy[i] ) continue;
    for ( int a = 0; a < 3; ++a ) {
      bytes::svarint( posS, q[3 * i + a] - mesh::predict( q, known, d.pred[i], last, a, qbits ) );
    }
    known[i] = 1;
    last = static_cast<int32_t>( i );
  }
  // Per-corner attributes and materials in decoded triangle order.
  int32_t pa = 0, pb = 0, pu = 0, pv = 0;
  for ( size_t k = 0; k < enc.triOrigin.size(); ++k ) {
    const int32_t t = enc.triOrigin[k];
    if ( t < 0 ) continue;
    matS.push_back( static_cast<char>( m.triMat.empty() ? 0 : m.triMat[t] ) );
    for ( int j = 0; j < 3; ++j ) {
      const size_t c = 3 * static_cast<size_t>( t ) + ( j + enc.triRotation[k] ) % 3;
      if ( !m.normals.empty() ) {
        auto n = mesh::octEncode( &m.normals[3 * c] );
        bytes::svarint( nrmS, n[0] - pa ); bytes::svarint( nrmS, n[1] - pb );
        pa = n[0]; pb = n[1];
      }
      if ( !m.uvs.empty() ) {
        const int32_t u = static_cast<int32_t>( std::lround( ( m.uvs[2 * c] - uvmin[0] ) / ( uvmax[0] - uvmin[0] ) * 65535.f ) );
        const int32_t v = static_cast<int32_t>( std::lround( ( m.uvs[2 * c + 1] - uvmin[1] ) / ( uvmax[1] - uvmin[1] ) * 65535.f ) );
        bytes::svarint( uvS, u - pu ); bytes::svarint( uvS, v - pv );
        pu = u; pv = v;
      }
    }
  }
  std::string o = "BMSH";
  bytes::u32( o, 1 );
  bytes::u32( o, nVerts );
  bytes::u32( o, nTris );
  bytes::u8( o, ( enc.conn.raw ? 1 : 0 ) | ( m.normals.empty() ? 0 : 2 ) | ( m.uvs.empty() ? 0 : 4 ) );
  bytes::u8( o, qbits );
  for ( float f : bmin ) bytes::f32( o, f );
  for ( float f : bmax ) bytes::f32( o, f );
  for ( float f : uvmin ) bytes::f32( o, f );
  for ( float f : uvmax ) bytes::f32( o, f );
  bytes::u32( o, enc.conn.nVerts );
  bytes::u16( o, m.materials.size() );
  for ( auto& mat : m.materials ) {
    bytes::u16( o, mat.name.size() ); o += mat.name;
    for ( float c : mat.rgba ) bytes::f32( o, c );
    bytes::u16( o, mat.texture.size() ); o += mat.texture;
  }
  size_t sz[7];
  sz[0] = stream( o, ops ); sz[1] = stream( o, offs ); sz[2] = stream( o, rawTris );
  sz[3] = stream( o, posS ); sz[4] = stream( o, nrmS ); sz[5] = stream( o, uvS ); sz[6] = stream( o, matS );
  if ( info ) {
    info->raw = enc.conn.raw;
    info->opsBytes = sz[0] + sz[1] + sz[2];
    info->posBytes = sz[3];
    info->nrmBytes = sz[4];
    info->uvBytes = sz[5];
    info->enc = std::move( enc );
  }
  return o;
}
