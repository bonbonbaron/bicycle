#include "data/Pack.h"
#include "data/Strip.h"
#include "data/tinfl.h"
#include "Bytes.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <sstream>
#include <unordered_map>

namespace assets {

namespace {
  struct Mount {
    std::string game;
    mem::Blob bytes{ mem::Allocator<char>( mem::PERMANENT ) };  // the whole pack (packs are small; the GM SoundFont isn't in it)
    std::unordered_map<std::string, Entry> entries;
    bool on{};
  };
  auto theMount() -> Mount& {
    static Mount m;
    return m;
  }
  std::mutex mtx;


  // "./test/genome/x.lua" -> "genome/x.lua" if it's under the mounted game's folder.
  auto packName( const std::string& path ) -> std::optional<std::string> {
    std::string p = path;
    for ( auto& c : p ) if ( c == '\\' ) c = '/';
    while ( p.rfind( "./", 0 ) == 0 ) p = p.substr( 2 );
    const auto& m = theMount();
    const std::string prefix = m.game + "/";
    if ( p.rfind( prefix, 0 ) != 0 ) return std::nullopt;
    p = p.substr( prefix.size() );
    // Collapse "a/./b" and "a//b".
    std::string out;
    std::stringstream ss( p );
    std::string part;
    while ( std::getline( ss, part, '/' ) ) {
      if ( part.empty() || part == "." ) continue;
      out += ( out.empty() ? "" : "/" ) + part;
    }
    return out;
  }
}

auto readFile( const std::string& path, mem::PoolId pool ) -> std::optional<mem::Blob> {
  std::ifstream f( path, std::ios::binary | std::ios::ate );
  if ( !f ) return std::nullopt;
  const auto size = static_cast<size_t>( f.tellg() );
  f.seekg( 0 );
  mem::Blob out( size, '\0', mem::Allocator<char>( pool ) );
  if ( size && !f.read( out.data(), static_cast<std::streamsize>( size ) ) ) return std::nullopt;
  return out;
}

auto inflateZlib( const uint8_t* src, size_t srcLen, size_t rawLen, mem::PoolId pool ) -> std::optional<mem::Blob> {
  mem::Blob out( rawLen, '\0', mem::Allocator<char>( pool ) );
  if ( rawLen == 0 ) return out;
  if ( !bicycleInflate( src, srcLen, reinterpret_cast<unsigned char*>( out.data() ), rawLen ) ) return std::nullopt;
  return out;
}

auto parsePack( std::string_view bytes, std::vector<Entry>& entries ) -> bool {
  bytes::Reader r( bytes );
  if ( r.str( 4 ) != "BPAK" || r.u32() != 1 ) return false;
  const uint32_t n = r.u32();
  for ( uint32_t i = 0; i < n && r.ok; ++i ) {
    Entry e;
    e.name = std::string( r.str( r.u16() ) );
    e.method = static_cast<Method>( r.u8() );
    e.rawLen = r.u32();
    e.dataLen = r.u32();
    e.offset = r.u32();
    if ( static_cast<size_t>( e.offset ) + e.dataLen > bytes.size() ) return false;
    entries.push_back( e );
  }
  return r.ok;
}

auto unpackEntry( std::string_view pack, const Entry& e, mem::PoolId pool ) -> std::optional<mem::Blob> {
  const auto* p = reinterpret_cast<const uint8_t*>( pack.data() + e.offset );
  switch ( e.method ) {
    case STORE:
      return mem::Blob( pack.substr( e.offset, e.dataLen ), mem::Allocator<char>( pool ) );
    case DEFLATE:
      return inflateZlib( p, e.dataLen, e.rawLen, pool );
    case STRIP: {
      auto img = strip::decode( pack.substr( e.offset, e.dataLen ), pool );
      if ( img && img->size() != e.rawLen ) return std::nullopt;
      return img;
    }
  }
  return std::nullopt;
}

auto mount( const std::string& gameName ) -> bool {
  std::unique_lock<std::mutex> l( mtx );
  auto& m = theMount();
  m = Mount{};
  m.game = gameName;
  if ( const char* loose = std::getenv( "BICYCLE_LOOSE_ASSETS" ); loose && *loose && *loose != '0' ) return false;
  auto bytes = readFile( gameName + "/assets.bpak", mem::PERMANENT );
  if ( !bytes ) return false;
  std::vector<Entry> entries;
  if ( !parsePack( *bytes, entries ) ) return false;
  m.bytes = std::move( *bytes );
  for ( auto& e : entries ) m.entries[e.name] = e;
  m.on = true;
  return true;
}

auto mounted() -> bool {
  return theMount().on;
}

auto read( const std::string& path, mem::PoolId pool ) -> std::optional<mem::Blob> {
  {
    std::unique_lock<std::mutex> l( mtx );
    auto& m = theMount();
    if ( m.on ) {
      if ( auto name = packName( path ) ) {
        if ( auto it = m.entries.find( *name ); it != m.entries.end() ) {
          return unpackEntry( m.bytes, it->second, pool );
        }
      }
    }
  }
  return readFile( path, pool );
}

auto list( const std::string& dir ) -> std::vector<std::string> {
  std::vector<std::string> out;
  std::unique_lock<std::mutex> l( mtx );
  auto& m = theMount();
  if ( !m.on ) return out;
  auto name = packName( dir );
  if ( !name ) return out;
  std::string prefix = *name + "/";
  std::string base = dir;
  while ( !base.empty() && ( base.back() == '/' || base.back() == '\\' ) ) base.pop_back();
  for ( auto& [entry, e] : m.entries ) {
    if ( entry.rfind( prefix, 0 ) == 0 && entry.find( '/', prefix.size() ) == std::string::npos ) {
      out.push_back( base + "/" + entry.substr( prefix.size() ) );
    }
  }
  std::sort( out.begin(), out.end() );
  return out;
}

auto exists( const std::string& path ) -> bool {
  {
    std::unique_lock<std::mutex> l( mtx );
    auto& m = theMount();
    if ( m.on ) {
      if ( auto name = packName( path ); name && m.entries.count( *name ) ) return true;
    }
  }
  std::ifstream f( path, std::ios::binary );
  return static_cast<bool>( f );
}

}  // namespace assets
