#include "v/common/PixelImages.h"
#include "data/Mem.h"
#include "data/Pack.h"
#include "v/common/Renderer.h"
#include "Constants.h"
#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>

namespace pixels {

auto load( const std::string& path ) -> std::optional<BImg> {
  mem::TempScope scratch;  // the file bytes are scratch; the parsed image lives in GRAPHIC
  auto bytes = assets::read( path, mem::TEMPORARY );
  if ( !bytes ) return std::nullopt;
  return BImg::parse( *bytes );
}

auto loadShared( const char* path ) -> std::shared_ptr<const BImg> {
  if ( !path || !*path ) return nullptr;
  auto img = load( path );
  return img ? std::make_shared<const BImg>( std::move( *img ) ) : nullptr;
}

namespace {

// Images loaded once and shared while anything draws them. Weak entries, so a scene's images still go when it
// unloads.
auto shared( const std::string& key, const std::function<std::optional<BImg>()>& make ) -> std::shared_ptr<const BImg> {
  static std::unordered_map<std::string, std::weak_ptr<const BImg>> cache;
  if ( auto it = cache.find( key ); it != cache.end() ) {
    if ( auto p = it->second.lock() ) return p;
  }
  auto img = make();
  if ( !img ) return nullptr;
  auto p = std::make_shared<const BImg>( std::move( *img ) );
  cache[key] = p;
  return p;
}

auto graybody( const std::string& path ) -> std::shared_ptr<const BImg> {
  return shared( path, [&] { return load( path ); } );
}

// "i:rgba,i:rgba,..." (decimal; RGBA with R in the low byte) -> palette entries to replace.
auto parsePalette( const std::string& spec ) -> std::vector<std::pair<int, uint32_t>> {
  std::vector<std::pair<int, uint32_t>> out;
  std::istringstream in( spec );
  std::string item;
  while ( std::getline( in, item, ',' ) ) {
    const auto colon = item.find( ':' );
    if ( colon == std::string::npos ) continue;
    out.emplace_back( std::stoi( item.substr( 0, colon ) ), static_cast<uint32_t>( std::stoul( item.substr( colon + 1 ) ) ) );
  }
  return out;
}

using Palette = decltype( BImg::palette );
auto recolored( Palette palette, const std::vector<std::pair<int, uint32_t>>& changes ) -> Palette {
  for ( auto [i, c] : changes ) {
    if ( i >= 0 && static_cast<size_t>( i ) < palette.size() ) palette[i] = c;
  }
  return palette;
}

// Draws the opaque pixels of src over dst (true color), clipped to both.
template <class PixelOf>
void stamp( BImg& dst, int w, int h, PixelOf&& pixelOf ) {
  for ( int y = 0; y < std::min( dst.h, h ); ++y )
    for ( int x = 0; x < std::min( dst.w, w ); ++x ) {
      const uint32_t p = pixelOf( x, y );
      if ( p >> 24 ) dst.rgba[static_cast<size_t>( y ) * dst.w + x] = p;
    }
}

auto joined( const std::vector<std::string>& layers ) -> std::string {
  std::string key;
  for ( const auto& l : layers ) key += l + "\n";
  return key;
}

}  // namespace

auto sprite( const std::vector<std::string>& layers ) -> std::shared_ptr<const BImg> {
  if ( layers.size() == 1 ) return graybody( layers[0] );  // many coins, one image
  std::optional<BImg> out;
  for ( const auto& path : layers ) {
    auto layer = load( path );
    if ( !layer ) return nullptr;
    if ( !out ) {
      out = std::move( *layer );
      continue;
    }
    if ( out->indexed() ) {  // a second layer: the result goes true color
      out->rgba.resize( static_cast<size_t>( out->w ) * out->h );
      for ( int y = 0; y < out->h; ++y )
        for ( int x = 0; x < out->w; ++x ) out->rgba[static_cast<size_t>( y ) * out->w + x] = out->pixel( x, y );
      out->palette.clear();
      out->indices.clear();
    }
    stamp( *out, layer->w, layer->h, [&]( int x, int y ) { return layer->pixel( x, y ); } );
  }
  if ( !out ) return nullptr;
  return std::make_shared<const BImg>( std::move( *out ) );
}

auto sprite( const std::vector<std::string>& layers, const std::string& palette ) -> std::shared_ptr<const BImg> {
  if ( palette.empty() ) return sprite( layers );
  for ( const auto& l : layers ) {
    auto gray = graybody( l );
    if ( !gray || !gray->indexed() ) return nullptr;
  }
  const auto changes = parsePalette( palette );
  if ( layers.size() == 1 ) {  // a palette variant: the graybody's indices, a new palette
    auto gray = graybody( layers[0] );
    return shared( layers[0] + "|" + palette, [&]() -> std::optional<BImg> {
      BImg v;
      v.w = gray->w;
      v.h = gray->h;
      v.palette = recolored( gray->palette, changes );
      v.gray = gray;
      return v;
    } );
  }
  // Composites are true color: recolor each layer, then stack them.
  return shared( joined( layers ) + "|" + palette, [&]() -> std::optional<BImg> {
    std::optional<BImg> out;
    for ( const auto& l : layers ) {
      auto gray = graybody( l );
      if ( !out ) {
        out.emplace();
        out->w = gray->w;
        out->h = gray->h;
        out->rgba.assign( static_cast<size_t>( gray->w ) * gray->h, 0u );
      }
      const auto p = recolored( gray->palette, changes );
      stamp( *out, gray->w, gray->h, [&]( int x, int y ) -> uint32_t {
        const uint8_t k = gray->indices[static_cast<size_t>( y ) * gray->w + x];
        return k ? p[k] : 0u;
      } );
    }
    return out;
  } );
}

auto tilemap( const BImg& set, const int* tiles, int nw, int nh, int tileW, int tileH ) -> BImg {
  BImg map;
  map.w = nw * tileW;
  map.h = nh * tileH;
  map.palette = set.palette;
  if ( set.indexed() ) map.indices.assign( static_cast<size_t>( map.w ) * map.h, 0 );
  else map.rgba.assign( static_cast<size_t>( map.w ) * map.h, 0 );
  const int nTiles = set.w / tileW;
  for ( int ty = 0; ty < nh; ++ty ) {
    for ( int tx = 0; tx < nw; ++tx ) {
      const int t = tiles[ty * nw + tx];
      if ( t <= 0 || t >= nTiles ) continue;  // tile 0 is Aseprite's empty tile
      for ( int py = 0; py < std::min( tileH, set.h ); ++py ) {
        for ( int px = 0; px < tileW; ++px ) {
          const size_t src = static_cast<size_t>( py ) * set.w + t * tileW + px;
          const size_t dst = static_cast<size_t>( ty * tileH + py ) * map.w + tx * tileW + px;
          if ( set.indexed() ) map.indices[dst] = set.indices[src];
          else map.rgba[dst] = set.rgba[src];
        }
      }
    }
  }
  return map;
}

auto asImage( std::shared_ptr<const BImg> px ) -> Image {
  Image img;
  img.size.w = ( 2 * px->w ) << FIXEDPT_DEC_BITS;
  img.size.h = px->h << FIXEDPT_DEC_BITS;
  img.size.d = 1 << FIXEDPT_DEC_BITS;
  gfx::renderer().preloadImage( px );  // into the pixel modes' texture atlas, at load
  img.pixels = std::move( px );
  return img;
}

auto splitLines( const char* paths ) -> std::vector<std::string> {
  std::vector<std::string> out;
  std::istringstream in( paths ? paths : "" );
  std::string line;
  while ( std::getline( in, line ) ) {
    if ( !line.empty() ) out.push_back( line );
  }
  return out;
}

}  // namespace pixels
