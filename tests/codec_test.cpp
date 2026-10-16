// Headless round-trip tests for the asset codecs: deflate/tinfl, strip images, Edgebreaker meshes, packs.
//   build/bicycle-tests [game folder ...]   (run by `ctest` and by tools/test_all.sh)
// With game folders, it also round-trips every file in their assets.bpak against the loose source files.
#include "data/Pack.h"
#include "data/Strip.h"
#include "data/BImg.h"
#include "data/Mesh.h"
#include "data/Edgebreaker.h"
#include "data/Atlas.h"
#include "data/Skin.h"
#include "data/Obj.h"
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
  // Per-vertex skin attributes (they must follow Edgebreaker's vertex reordering).
  m.joints.clear(); m.weights.clear();
  for ( size_t i = 0; i < m.nVerts(); ++i ) {
    const uint8_t w0 = uint8_t( 100 + i % 120 ), w1 = uint8_t( 255 - w0 );
    m.joints.insert( m.joints.end(), { uint8_t( i % 7 ), uint8_t( ( i / 7 ) % 7 ), 0, 0 } );
    m.weights.insert( m.weights.end(), { w0, w1, 0, 0 } );
  }
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
  if ( !m.joints.empty() ) {
    CHECK( d->joints.size() == m.joints.size() && d->weights.size() == m.weights.size(), "%s: skin attribute counts", name );
    size_t bad = 0;
    for ( size_t i = 0; i < d->nVerts() && d->joints.size() == m.joints.size(); ++i ) {
      for ( int k = 0; k < 4; ++k ) {
        bad += d->joints[4 * i + k] != m.joints[4 * origOf[i] + k] || d->weights[4 * i + k] != m.weights[4 * origOf[i] + k];
      }
    }
    CHECK( bad == 0, "%s: %zu skin joints/weights didn't follow their vertex", name, bad );
  }
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
    else if ( ext == ".bskin" ) {  // packed quantized (lossy): same rig, poses within tolerance
      auto src = readFile( game + "/" + e.name );
      auto a = src ? skin::decode( *src ) : std::nullopt;
      auto b = skin::decode( *data );
      CHECK( a && b && a->joints.size() == b->joints.size() && a->actions.size() == b->actions.size(), "%s: skin differs", e.name.c_str() );
      if ( a && b && a->joints.size() == b->joints.size() && a->actions.size() == b->actions.size() ) {
        double err = 0;
        std::vector<float> pa( 16 * a->joints.size() ), pb( pa.size() );
        for ( size_t k = 0; k < a->actions.size(); ++k ) {
          for ( float t = 0; t < 2; t += 0.137f ) {
            skin::pose( *a, a->actions[k], t, pa.data() );
            skin::pose( *b, b->actions[k], t, pb.data() );
            for ( size_t i = 0; i < pa.size(); ++i ) err = std::max( err, double( std::fabs( pa[i] - pb[i] ) ) );
          }
        }
        CHECK( err < 5e-3, "%s: pose error %g after packing", e.name.c_str(), err );
        std::printf( "  %-32s %zu joints %zu actions  max pose err %.2g after quantization\n", e.name.c_str(), b->joints.size(), b->actions.size(), err );
        byType[ext].first += src->size();
      }
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

// The atlas port: every image placed inside the atlas, none overlapping, for many random sets.
static void atlasTests() {
  std::mt19937 rng( 7 );
  double worstFill = 1;
  for ( int trial = 0; trial < 300; ++trial ) {
    const int n = 1 + static_cast<int>( rng() % 60 );
    std::vector<atlas::Size> sizes;
    uint64_t area = 0;
    for ( int i = 0; i < n; ++i ) {
      const bool strip = rng() % 4 == 0;  // animation strips are wide
      atlas::Size sz{ 1 + rng() % ( strip ? 200 : 48 ), 1 + rng() % 48 };
      sizes.push_back( sz );
      area += uint64_t( sz.w + 1 ) * ( sz.h + 1 );
    }
    auto a = atlas::atlasNew( sizes, 1 );
    atlas::atlasPlanPlacements( a );
    const auto rects = atlas::atlasPlacements( a, 1 );
    bool ok = true;
    for ( int i = 0; i < n && ok; ++i ) {
      const auto& r = rects[i];
      ok &= r.w == int( sizes[i].w ) && r.h == int( sizes[i].h ) && r.x >= 0 && r.y >= 0 &&
            uint32_t( r.x + r.w ) <= a.width() && uint32_t( r.y + r.h ) <= a.height();
      for ( int j = 0; j < i && ok; ++j ) {
        const auto& q = rects[j];
        ok &= r.x + r.w + 1 <= q.x || q.x + q.w + 1 <= r.x || r.y + r.h + 1 <= q.y || q.y + q.h + 1 <= r.y;
      }
    }
    CHECK( ok, "atlas trial %d (%d images): out of bounds or overlapping", trial, n );
    worstFill = std::min( worstFill, double( area ) / ( double( a.width() ) * a.height() ) );
  }
  std::printf( "atlas: 300 random sets packed without overlap; worst fill %.0f%%\n", worstFill * 100 );
}

// .bskin: quantized keyframes round-trip within tolerance, poses stay close; the real exported rig decodes.
static void testSkins() {
  Skin s;
  s.joints = { { "root", -1, {} }, { "a", 0, {} }, { "b", 1, {} } };
  for ( auto& j : s.joints ) j.inverseBind = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, -0.5f, 0, 1 };
  std::mt19937 rng( 7 );
  std::uniform_real_distribution<float> u( -1, 1 );
  SkinAction a;
  a.name = "walk";
  a.nFrames = 24;
  for ( uint32_t f = 0; f < a.nFrames; ++f ) {
    for ( size_t j = 0; j < s.joints.size(); ++j ) {
      const float ang = 0.7f * std::sin( f * 0.26f + j ), ax[3] = { u( rng ), u( rng ), u( rng ) };
      const float l = std::sqrt( ax[0] * ax[0] + ax[1] * ax[1] + ax[2] * ax[2] ) + 1e-6f;
      a.t.insert( a.t.end(), { 0.1f * f, 0.5f * j, u( rng ) * 0.2f } );
      a.q.insert( a.q.end(), { std::sin( ang / 2 ) * ax[0] / l, std::sin( ang / 2 ) * ax[1] / l, std::sin( ang / 2 ) * ax[2] / l, std::cos( ang / 2 ) } );
    }
  }
  s.actions.push_back( a );
  for ( bool quant : { false, true } ) {
    const auto bytes = skin::encode( s, quant, false );
    auto d = skin::decode( bytes );
    CHECK( d && d->joints.size() == 3 && d->actions.size() == 1 && d->action( "walk" ), "skin (%s): decode", quant ? "quantized" : "float" );
    if ( !d || !d->action( "walk" ) ) continue;
    const SkinAction& b = *d->action( "walk" );
    double tErr = 0, qErr = 0, mErr = 0;
    for ( size_t i = 0; i < a.t.size() && i < b.t.size(); ++i ) tErr = std::max( tErr, double( std::fabs( a.t[i] - b.t[i] ) ) );
    for ( size_t i = 0; i + 3 < a.q.size() && i + 3 < b.q.size(); i += 4 ) {
      double dot = 0;
      for ( int k = 0; k < 4; ++k ) dot += a.q[i + k] * b.q[i + k];
      qErr = std::max( qErr, 1 - std::fabs( dot ) );  // same rotation if q == +-q'
    }
    float pa[16 * 3], pb[16 * 3];
    for ( float t : { 0.0f, 0.31f, 0.77f } ) {
      skin::pose( s, a, t, pa );
      skin::pose( *d, b, t, pb );
      for ( int i = 0; i < 48; ++i ) mErr = std::max( mErr, double( std::fabs( pa[i] - pb[i] ) ) );
    }
    CHECK( tErr < 1e-3 && qErr < 1e-5 && mErr < 5e-3, "skin (%s): t err %g, q err %g, pose err %g", quant ? "quantized" : "float", tErr, qErr, mErr );
    std::printf( "  skin %-9s %zu bytes  t err %.2g  q err %.2g  pose err %.2g\n", quant ? "quantized" : "float", bytes.size(), tErr, qErr, mErr );
  }
  // The rig scripts/blender/export.py wrote, and its mesh.
  if ( auto f = readFile( "test/genome/model/pilgrim3d.bskin" ) ) {
    auto d = skin::decode( *f );
    CHECK( d && d->joints.size() == 7 && d->action( "idle" ) && d->action( "walk" ), "pilgrim3d.bskin: decode / actions" );
    MeshData m;
    std::string err;
    if ( d && readObj( "test/genome/model/pilgrim3d.obj", m, err ) ) {
      CHECK( applySkinVertices( m, *d ), "pilgrim3d: .bskin vertices don't match the OBJ" );
      size_t badSum = 0;
      for ( size_t i = 0; i + 3 < m.weights.size(); i += 4 ) badSum += m.weights[i] + m.weights[i + 1] + m.weights[i + 2] + m.weights[i + 3] != 255;
      CHECK( badSum == 0, "pilgrim3d: %zu vertices' weights don't sum to 255", badSum );
      meshRoundTrip( "pilgrim3d+skin", m, mesh::DEFAULT_QBITS, -1, false );
    }
  }
  else {
    CHECK( false, "test/genome/model/pilgrim3d.bskin missing" );
  }
}

int main( int argc, char** argv ) {
  atlasTests();
  testDeflate();
  testStrip();
  testMeshes();
  testSkins();
  for ( int i = 1; i < argc; ++i ) testPack( argv[i] );
  std::printf( failures ? "%d FAILURES\n" : "ALL CODEC TESTS PASSED\n", failures );
  return failures ? 1 : 0;
}
