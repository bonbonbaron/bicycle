// Headless round-trip tests for the asset codecs: deflate/tinfl, strip images, Edgebreaker meshes, packs.
//   build/bicycle-tests [game folder ...]   (run by `ctest` and by tools/test_all.sh)
// With game folders, it also round-trips every file in their assets.bpak against the loose source files.
#include "data/Pack.h"
#include "data/Strip.h"
#include "data/BImg.h"
#include "data/Mesh.h"
#include "data/Edgebreaker.h"
#include "../tools/pack/Deflate.h"
#include "../tools/pack/StripEncode.h"
#include "../tools/pack/MeshEncode.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <sstream>

static int failures = 0;
#define CHECK( cond, ... ) do { if ( !( cond ) ) { ++failures; std::printf( "FAIL %s:%d: ", __FILE__, __LINE__ ); std::printf( __VA_ARGS__ ); std::printf( "\n" ); } } while ( 0 )

static void testDeflate() {
  std::mt19937 rng( 1 );
  for ( size_t n : { 0u, 1u, 100u, 70000u } ) {
    std::string s( n, '\0' );
    for ( auto& c : s ) c = static_cast<char>( "abcd"[rng() % 4] );
    const auto z = deflateZlib( s );
    const auto back = assets::inflateZlib( reinterpret_cast<const uint8_t*>( z.data() ), z.size(), s.size() );
    CHECK( back && std::string_view( *back ) == s, "deflate round trip of %zu bytes", n );
  }
  std::printf( "deflate/tinfl: ok\n" );
}

static auto stripRoundTrip( const BImg& img, int n ) -> bool {
  const auto enc = stripEncode( img, n );
  const auto dec = strip::decode( enc );
  return dec && std::string_view( *dec ) == img.serialize();
}

static void testStrip() {
  std::mt19937 rng( 2 );
  for ( int colors : { 2, 3, 4, 5, 16, 17, 200 } ) {
    BImg img;
    img.w = 37; img.h = 13;
    for ( int i = 0; i < colors; ++i ) img.palette.push_back( rng() );
    for ( int i = 0; i < img.w * img.h; ++i ) img.indices.push_back( ( rng() % 3 == 0 ) ? rng() % colors : 0 );
    for ( int n : { 4, 8, 16, 32 } ) CHECK( stripRoundTrip( img, n ), "strip %d colors %d units", colors, n );
  }
  std::printf( "strip codec: ok\n" );
}

// --- meshes ---
static auto icosphere( int subdiv ) -> MeshData {
  const float t = ( 1 + std::sqrt( 5.f ) ) / 2;
  MeshData m;
  m.pos = { -1, t, 0, 1, t, 0, -1, -t, 0, 1, -t, 0, 0, -1, t, 0, 1, t, 0, -1, -t, 0, 1, -t, t, 0, -1, t, 0, 1, -t, 0, -1, -t, 0, 1 };
  m.tris = { 0,11,5, 0,5,1, 0,1,7, 0,7,10, 0,10,11, 1,5,9, 5,11,4, 11,10,2, 10,7,6, 7,1,8,
             3,9,4, 3,4,2, 3,2,6, 3,6,8, 3,8,9, 4,9,5, 2,4,11, 6,2,10, 8,6,7, 9,8,1 };
  for ( int s = 0; s < subdiv; ++s ) {
    std::map<std::pair<uint32_t, uint32_t>, uint32_t> mid;
    auto midpoint = [&]( uint32_t a, uint32_t b ) {
      auto k = std::minmax( a, b );
      auto it = mid.find( k );
      if ( it != mid.end() ) return it->second;
      const uint32_t i = m.pos.size() / 3;
      for ( int c = 0; c < 3; ++c ) m.pos.push_back( ( m.pos[3 * a + c] + m.pos[3 * b + c] ) / 2 );
      mid[k] = i;
      return i;
    };
    std::vector<uint32_t> nt;
    for ( size_t i = 0; i < m.tris.size(); i += 3 ) {
      const uint32_t a = m.tris[i], b = m.tris[i + 1], c = m.tris[i + 2];
      const uint32_t ab = midpoint( a, b ), bc = midpoint( b, c ), ca = midpoint( c, a );
      nt.insert( nt.end(), { a, ab, ca, b, bc, ab, c, ca, bc, ab, bc, ca } );
    }
    m.tris.assign( nt.begin(), nt.end() );
  }
  return m;
}

static auto grid( int w, int h ) -> MeshData {  // open: one hole (its border)
  MeshData m;
  for ( int y = 0; y <= h; ++y ) for ( int x = 0; x <= w; ++x ) m.pos.insert( m.pos.end(), { float( x ), float( y ), 0.1f * ( ( x * y ) % 3 ) } );
  for ( int y = 0; y < h; ++y ) for ( int x = 0; x < w; ++x ) {
    const uint32_t a = y * ( w + 1 ) + x, b = a + 1, c = a + w + 1, d = c + 1;
    m.tris.insert( m.tris.end(), { a, b, d, a, d, c } );
  }
  return m;
}

static auto torus( int n, int k ) -> MeshData {  // genus 1: Edgebreaker falls back to raw indices
  MeshData m;
  for ( int i = 0; i < n; ++i ) for ( int j = 0; j < k; ++j ) {
    const float u = 6.2831853f * i / n, v = 6.2831853f * j / k;
    m.pos.insert( m.pos.end(), { ( 2 + std::cos( v ) ) * std::cos( u ), ( 2 + std::cos( v ) ) * std::sin( u ), std::sin( v ) } );
  }
  for ( int i = 0; i < n; ++i ) for ( int j = 0; j < k; ++j ) {
    const uint32_t a = i * k + j, b = ( ( i + 1 ) % n ) * k + j, c = i * k + ( j + 1 ) % k, d = ( ( i + 1 ) % n ) * k + ( j + 1 ) % k;
    m.tris.insert( m.tris.end(), { a, b, d, a, d, c } );
  }
  return m;
}

static auto merge( MeshData a, const MeshData& b ) -> MeshData {
  const uint32_t off = a.pos.size() / 3;
  a.pos.insert( a.pos.end(), b.pos.begin(), b.pos.end() );
  for ( auto t : b.tris ) a.tris.push_back( t + off );
  return a;
}

static void addAttributes( MeshData& m ) {
  m.normals.clear(); m.uvs.clear(); m.triMat.clear();
  for ( size_t c = 0; c < m.tris.size(); ++c ) {
    const float* p = &m.pos[3 * m.tris[c]];
    const float n[3] = { p[0] + 0.3f, p[1] - 0.2f, p[2] + 1.0f };
    const float l = std::sqrt( n[0] * n[0] + n[1] * n[1] + n[2] * n[2] );
    m.normals.insert( m.normals.end(), { n[0] / l, n[1] / l, n[2] / l } );
    m.uvs.insert( m.uvs.end(), { p[0] * 0.25f, p[1] * 0.5f } );
  }
  for ( size_t t = 0; t < m.tris.size() / 3; ++t ) m.triMat.push_back( t % 2 );
  m.materials = { MeshMaterial{ "a" }, MeshMaterial{ "b" } };
}

// Returns the maximum position error, or -1 on a mismatch.
// expectEdgebreaker: 1 must use Edgebreaker, 0 must fall back to raw, -1 either. Synthetic meshes get synthetic
// normals/uvs/materials; real ones (Blender exports) keep theirs.
static auto meshRoundTrip( const char* name, MeshData m, int qbits, int expectEdgebreaker, bool synthAttributes = true ) -> double {
  if ( synthAttributes ) addAttributes( m );
  MeshEncodeInfo info;
  const auto bytes = meshEncode( m, qbits, &info );
  auto d = mesh::decode( bytes );
  CHECK( d.has_value(), "%s: decode failed", name );
  if ( !d ) return -1;
  CHECK( d->nVerts() == m.nVerts() && d->nTris() == m.nTris(), "%s: counts %zu/%zu vs %zu/%zu", name, d->nVerts(), d->nTris(), m.nVerts(), m.nTris() );
  CHECK( expectEdgebreaker < 0 || info.raw != bool( expectEdgebreaker ), "%s: Edgebreaker %s", name, info.raw ? "fell back to raw" : "was used" );
  // Decoded vertex i (compacted) -> original vertex.
  std::vector<int32_t> origOf;
  for ( auto o : info.enc.vertexOrigin ) if ( o >= 0 ) origOf.push_back( o );
  if ( origOf.size() != d->nVerts() ) { CHECK( false, "%s: vertex map size", name ); return -1; }
  // Same triangles (as rotation-normalised triples of original ids).
  auto norm = []( uint32_t a, uint32_t b, uint32_t c ) {
    std::array<uint32_t, 3> t{ a, b, c };
    while ( t[0] != std::min( { a, b, c } ) ) std::rotate( t.begin(), t.begin() + 1, t.end() );
    return t;
  };
  std::multiset<std::array<uint32_t, 3>> want, got;
  for ( size_t i = 0; i < m.tris.size(); i += 3 ) want.insert( norm( m.tris[i], m.tris[i + 1], m.tris[i + 2] ) );
  for ( size_t i = 0; i < d->tris.size(); i += 3 ) got.insert( norm( origOf[d->tris[i]], origOf[d->tris[i + 1]], origOf[d->tris[i + 2]] ) );
  CHECK( want == got, "%s: triangles differ", name );
  // Positions within half a quantization step; per-corner attributes follow their corners.
  double maxErr = 0, maxAllowed = 0;
  float bmin[3] = { 1e30f, 1e30f, 1e30f }, bmax[3] = { -1e30f, -1e30f, -1e30f };
  for ( size_t i = 0; i < m.nVerts(); ++i ) for ( int a = 0; a < 3; ++a ) { bmin[a] = std::min( bmin[a], m.pos[3 * i + a] ); bmax[a] = std::max( bmax[a], m.pos[3 * i + a] ); }
  for ( int a = 0; a < 3; ++a ) maxAllowed = std::max( maxAllowed, 0.5 * ( bmax[a] - bmin[a] ) / ( ( 1 << qbits ) - 1 ) * 1.0001 + 1e-6 );
  for ( size_t i = 0; i < d->nVerts(); ++i ) for ( int a = 0; a < 3; ++a ) maxErr = std::max( maxErr, double( std::fabs( d->pos[3 * i + a] - m.pos[3 * origOf[i] + a] ) ) );
  CHECK( maxErr <= maxAllowed, "%s: position error %g > %g", name, maxErr, maxAllowed );
  size_t k = 0;
  double nErr = 0, uvErr = 0;
  for ( size_t t = 0; t < info.enc.triOrigin.size(); ++t ) {
    const int32_t o = info.enc.triOrigin[t];
    if ( o < 0 ) continue;
    CHECK( m.triMat.empty() || d->triMat[k] == m.triMat[o], "%s: material of triangle %zu", name, k );
    for ( int j = 0; j < 3; ++j ) {
      const size_t oc = 3 * o + ( j + info.enc.triRotation[t] ) % 3, dc = 3 * k + j;
      if ( !m.normals.empty() ) for ( int a = 0; a < 3; ++a ) nErr = std::max( nErr, double( std::fabs( d->normals[3 * dc + a] - m.normals[3 * oc + a] ) ) );
      if ( !m.uvs.empty() ) for ( int a = 0; a < 2; ++a ) uvErr = std::max( uvErr, double( std::fabs( d->uvs[2 * dc + a] - m.uvs[2 * oc + a] ) ) );
    }
    ++k;
  }
  // uv tolerance: 16-bit quantization over the uv bounding box
  float uvSpan = 0;
  for ( size_t i = 0; i + 1 < m.uvs.size(); i += 2 ) uvSpan = std::max( { uvSpan, std::fabs( m.uvs[i] ), std::fabs( m.uvs[i + 1] ) } );
  CHECK( nErr < 0.01 && uvErr < 0.001 * std::max( 1.0f, 2 * uvSpan ), "%s: normal error %g, uv error %g", name, nErr, uvErr );
  const double raw = m.nVerts() * 12.0 + m.nTris() * 12.0;
  std::printf( "  %-16s %6zu verts %6zu tris  %s  conn %.2f bits/tri  pos %.1f bits/vert  max err %.2g (allowed %.2g)  %zu -> %zu bytes geometry+conn (%.1fx)\n",
               name, m.nVerts(), m.nTris(), info.raw ? "raw" : "EB ", 8.0 * info.opsBytes / m.nTris(), 8.0 * info.posBytes / m.nVerts(),
               maxErr, maxAllowed, size_t( raw ), info.opsBytes + info.posBytes, raw / ( info.opsBytes + info.posBytes ) );
  return maxErr;
}

static void testMeshes() {
  std::printf( "edgebreaker meshes:\n" );
  meshRoundTrip( "icosahedron", icosphere( 0 ), 14, 1 );
  meshRoundTrip( "icosphere(4)", icosphere( 4 ), 14, 1 );
  meshRoundTrip( "grid 30x20", grid( 30, 20 ), 12, 1 );
  meshRoundTrip( "two islands", merge( icosphere( 2 ), grid( 5, 5 ) ), 14, 1 );
  meshRoundTrip( "torus (genus 1)", torus( 24, 12 ), 14, 0 );
  // Edgebreaker's decoder alone on a hand-made connectivity: a tetrahedron is C,C? No: init + C... just check
  // that a corrupted stream is rejected rather than crashing.
  eb::Connectivity bad;
  bad.nVerts = 4;
  bad.ops = { eb::S, eb::S };
  eb::Decoded d;
  CHECK( !eb::decode( bad, d ), "corrupt connectivity accepted" );
}

static auto readFile( const std::string& p ) -> std::optional<std::string> {
  std::ifstream f( p, std::ios::binary );
  if ( !f ) return std::nullopt;
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

// Every entry of <game>/assets.bpak must inflate to its source file (meshes: decode to the source's counts).
static void testPack( const std::string& game ) {
  auto pack = readFile( game + "/assets.bpak" );
  CHECK( pack.has_value(), "no %s/assets.bpak (build the 'assets' target)", game.c_str() );
  if ( !pack ) return;
  std::vector<assets::Entry> entries;
  CHECK( assets::parsePack( *pack, entries ), "%s: bad pack", game.c_str() );
  std::map<std::string, std::pair<size_t, size_t>> byType;  // extension -> raw, packed
  int n = 0;
  for ( auto& e : entries ) {
    auto data = assets::unpackEntry( *pack, e );
    CHECK( data.has_value(), "%s: can't unpack", e.name.c_str() );
    if ( !data ) continue;
    const auto dot = e.name.find_last_of( '.' );
    const std::string ext = dot == std::string::npos ? "" : e.name.substr( dot );
    if ( ext == ".bmesh" ) {
      auto m = mesh::decode( *data );
      CHECK( m.has_value(), "%s: mesh doesn't decode", e.name.c_str() );
      auto src = game + "/" + e.name.substr( 0, e.name.size() - 6 ) + ".obj";
      MeshData orig;
      std::string err;
      if ( m && readObj( src, orig, err ) ) {
        CHECK( m->nVerts() == orig.nVerts() && m->nTris() == orig.nTris(), "%s: counts differ from %s", e.name.c_str(), src.c_str() );
        // Full round trip of the real (e.g. Blender-exported) mesh: triangles, positions within quantization,
        // per-corner normals and uvs, materials.
        meshRoundTrip( e.name.c_str(), orig, mesh::DEFAULT_QBITS, -1, false );
      }
      byType[ext].first += readFile( src ).value_or( "" ).size();
    }
    else {
      auto src = readFile( game + "/" + e.name );
      CHECK( src.has_value(), "%s: no source file", e.name.c_str() );
      if ( src ) {
        CHECK( std::string_view( *src ) == std::string_view( *data ), "%s: unpacked bytes differ from the source", e.name.c_str() );
        byType[ext].first += src->size();
      }
    }
    byType[ext].second += e.dataLen;
    ++n;
  }
  std::printf( "%s/assets.bpak: %d entries round-trip\n", game.c_str(), n );
  for ( auto& [ext, sz] : byType ) {
    std::printf( "  %-8s %9zu -> %9zu bytes  (%.1fx)\n", ext.c_str(), sz.first, sz.second, sz.second ? double( sz.first ) / sz.second : 0.0 );
  }
}

int main( int argc, char** argv ) {
  testDeflate();
  testStrip();
  testMeshes();
  for ( int i = 1; i < argc; ++i ) testPack( argv[i] );
  std::printf( failures ? "%d FAILURES\n" : "ALL CODEC TESTS PASSED\n", failures );
  return failures ? 1 : 0;
}
