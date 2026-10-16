// bicycle-pack: compresses a game's assets into <game>/assets.bpak (see include/data/Pack.h).
//   bicycle-pack <game folder> [<output>]      default output: <game folder>/assets.bpak
// Packs <game>/main.lua and everything under <game>/genome:
//   .bimg          indexed: strip-coded (jollybean's strips) or deflated, whichever is smaller; true color: deflated
//   models/*.obj   Edgebreaker-coded into models/<name>.bmesh (with its .mtl); the .obj/.mtl aren't packed
//   .png next to a .bimg of the same name (old exports) and .mtl: skipped
//   anything else  deflated, or stored if deflate doesn't help
// Prints each entry's method and sizes and totals per file type.
#include "data/Pack.h"
#include "data/BImg.h"
#include "data/Mesh.h"
#include "Deflate.h"
#include "StripEncode.h"
#include "MeshEncode.h"
#include "../../src/data/Bytes.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

namespace fs = std::filesystem;

static auto readFile( const fs::path& p ) -> std::string {
  std::ifstream f( p, std::ios::binary );
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

struct Packed {
  std::string name;
  assets::Method method{};
  uint32_t rawLen{};
  std::string data;
  std::string note;
  size_t srcLen{};
};

static auto packGeneric( const std::string& name, const std::string& raw ) -> Packed {
  Packed p{ name, assets::DEFLATE, static_cast<uint32_t>( raw.size() ), deflateZlib( raw ), "", raw.size() };
  if ( p.data.size() >= raw.size() ) {
    p.method = assets::STORE;
    p.data = raw;
  }
  return p;
}

static auto packImage( const std::string& name, const std::string& raw ) -> Packed {
  Packed best = packGeneric( name, raw );
  auto img = BImg::parse( raw );
  if ( !img ) {
    best.note = "not a valid .bimg";
    return best;
  }
  if ( img->indexed() ) {
    for ( int n : { 4, 8, 16, 32 } ) {
      std::string enc = stripEncode( *img, n );
      if ( enc.size() < best.data.size() ) {
        best.method = assets::STRIP;
        best.data = enc;
        best.note = "strips of " + std::to_string( n );
      }
    }
  }
  return best;
}

int main( int argc, char** argv ) {
  if ( argc < 2 ) {
    std::fprintf( stderr, "usage: bicycle-pack <game folder> [<output .bpak>]\n" );
    return 2;
  }
  const fs::path game = argv[1];
  const fs::path out = argc > 2 ? fs::path( argv[2] ) : game / "assets.bpak";
  std::vector<fs::path> files;
  if ( fs::exists( game / "main.lua" ) ) files.push_back( game / "main.lua" );
  if ( fs::exists( game / "genome" ) ) {
    for ( auto& e : fs::recursive_directory_iterator( game / "genome" ) ) {
      if ( e.is_regular_file() ) files.push_back( e.path() );
    }
  }
  std::sort( files.begin(), files.end() );
  std::vector<Packed> packed;
  for ( auto& f : files ) {
    const std::string rel = fs::relative( f, game ).generic_string();
    const std::string ext = f.extension().string();
    if ( ext == ".mtl" || ext == ".blend" || ext == ".blend1" ) continue;
    if ( ext == ".png" && fs::exists( fs::path( f ).replace_extension( ".bimg" ) ) ) continue;
    if ( ext == ".obj" ) {
      MeshData m;
      std::string err;
      if ( !readObj( f.string(), m, err ) ) {
        std::fprintf( stderr, "bicycle-pack: %s\n", err.c_str() );
        return 1;
      }
      // Textures: point at the .bimg the Blender exporter wrote next to the image.
      for ( auto& mat : m.materials ) {
        if ( mat.texture.empty() ) continue;
        fs::path tex = f.parent_path() / fs::path( mat.texture ).filename();
        tex.replace_extension( ".bimg" );
        mat.texture = fs::exists( tex ) ? fs::relative( tex, game ).generic_string() : "";
      }
      MeshEncodeInfo info;
      const std::string enc = meshEncode( m, mesh::DEFAULT_QBITS, &info );
      Packed p{ rel.substr( 0, rel.size() - 4 ) + ".bmesh", assets::STORE, static_cast<uint32_t>( enc.size() ), enc, "", fs::file_size( f ) };
      char note[200];
      std::snprintf( note, sizeof note, "%s, %zu verts %zu tris, conn %.2f bits/tri, pos %.1f bits/vert",
                     info.raw ? "raw indices (not Edgebreaker-able)" : "Edgebreaker", m.nVerts(), m.nTris(),
                     8.0 * info.opsBytes / std::max<size_t>( 1, m.nTris() ), 8.0 * info.posBytes / std::max<size_t>( 1, m.nVerts() ) );
      p.note = note;
      packed.push_back( p );
      continue;
    }
    const std::string raw = readFile( f );
    packed.push_back( ext == ".bimg" ? packImage( rel, raw ) : packGeneric( rel, raw ) );
  }
  // Write the pack.
  std::string header = "BPAK";
  bytes::u32( header, 1 );
  bytes::u32( header, packed.size() );
  size_t headerLen = header.size();
  for ( auto& p : packed ) headerLen += 2 + p.name.size() + 1 + 4 + 4 + 4;
  uint32_t offset = static_cast<uint32_t>( headerLen );
  std::string body;
  for ( auto& p : packed ) {
    bytes::u16( header, p.name.size() );
    header += p.name;
    bytes::u8( header, p.method );
    bytes::u32( header, p.rawLen );
    bytes::u32( header, p.data.size() );
    bytes::u32( header, offset );
    offset += p.data.size();
    body += p.data;
  }
  const std::string all = header + body;
  if ( !fs::exists( out ) || readFile( out ) != all ) {
    std::ofstream o( out, std::ios::binary );
    o << all;
    if ( !o ) {
      std::fprintf( stderr, "bicycle-pack: can't write %s\n", out.string().c_str() );
      return 1;
    }
  }
  // Report.
  static const char* methodName[] = { "store", "deflate", "strip" };
  std::map<std::string, std::pair<size_t, size_t>> byType;
  size_t totalSrc = 0, totalOut = 0;
  for ( auto& p : packed ) {
    const auto dot = p.name.find_last_of( '.' );
    auto& t = byType[dot == std::string::npos ? "" : p.name.substr( dot )];
    t.first += p.srcLen;
    t.second += p.data.size();
    totalSrc += p.srcLen;
    totalOut += p.data.size();
    if ( std::getenv( "BICYCLE_PACK_VERBOSE" ) ) {
      std::printf( "  %-48s %-7s %8zu -> %8zu  %s\n", p.name.c_str(), methodName[p.method], p.srcLen, p.data.size(), p.note.c_str() );
    }
    else if ( p.name.size() > 6 && p.name.substr( p.name.size() - 6 ) == ".bmesh" ) {
      std::printf( "  %s: %s\n", p.name.c_str(), p.note.c_str() );
    }
  }
  for ( auto& [ext, t] : byType ) {
    std::printf( "  %-8s %9zu -> %9zu bytes (%.1fx)\n", ext.c_str(), t.first, t.second, t.second ? double( t.first ) / t.second : 0.0 );
  }
  std::printf( "bicycle-pack: %s: %zu files, %zu -> %zu bytes (%.1fx)\n", out.string().c_str(), packed.size(), totalSrc,
               all.size(), all.size() ? double( totalSrc ) / all.size() : 0.0 );
  return 0;
}
