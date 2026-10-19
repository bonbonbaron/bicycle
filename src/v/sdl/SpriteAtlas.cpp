#include "v/sdl/SpriteAtlas.h"
#include "data/Atlas.h"
#include "data/BImg.h"
#include <cstdio>
#include <cstdlib>

namespace gfx {

void SpriteAtlas::add( const std::shared_ptr<const BImg>& img ) {
  if ( !img || _idx.count( img.get() ) ) return;
  _idx[img.get()] = _imgs.size();
  _imgs.push_back( img );
  _dirty = true;
}

auto SpriteAtlas::entry( const std::shared_ptr<const BImg>& img ) -> Entry {
  add( img );
  rebuildIfDirty();
  return _entries[_idx[img.get()]];
}

// jollybean's xRender start-up: atlasNew (sort), atlasPlanPlacements, assembleTextureAtlas, updateCmSrcRectIndices.
void SpriteAtlas::rebuildIfDirty() {
  if ( !_dirty ) return;
  _dirty = false;
  std::vector<atlas::Size> sizes;
  sizes.reserve( _imgs.size() );
  for ( const auto& i : _imgs ) sizes.push_back( { static_cast<uint32_t>( i->w ), static_cast<uint32_t>( i->h ) } );
  auto a = atlas::atlasNew( sizes, PAD );
  atlas::atlasPlanPlacements( a );
  const auto rects = atlas::atlasPlacements( a, PAD );
  _w = static_cast<int>( a.width() );
  _h = static_cast<int>( a.height() );
  _px.assign( static_cast<std::size_t>( _w ) * _h, 0u );
  _entries.resize( _imgs.size() );
  for ( std::size_t i = 0; i < _imgs.size(); ++i ) {
    const BImg& img = *_imgs[i];
    const auto& r = rects[i];
    _entries[i] = Entry{ r.x, r.y, r.w, r.h };
    for ( int y = 0; y < img.h; ++y ) {
      uint32_t* dst = _px.data() + static_cast<std::size_t>( r.y + y ) * _w + r.x;
      for ( int x = 0; x < img.w; ++x ) {
        const uint32_t rgba = img.pixel( x, y );  // R in the low byte
        dst[x] = ( rgba & 0xFF000000u ) | ( ( rgba & 0xFF ) << 16 ) | ( rgba & 0xFF00 ) | ( ( rgba >> 16 ) & 0xFF );
      }
    }
  }
  ++_version;
  if ( std::getenv( "BICYCLE_ATLAS_INFO" ) ) {
    std::fprintf( stderr, "bicycle: sprite atlas %dx%d, %zu images (rebuild %u)\n", _w, _h, _imgs.size(), _version );
  }
}

void SpriteAtlas::prune() {
  // Until nothing changes: a palette variant holds its graybody (BImg::gray), so dropping the variant can leave
  // the graybody with only the atlas's reference.
  const std::size_t before = _imgs.size();
  for ( bool dropped = true; dropped; ) {
    std::vector<std::shared_ptr<const BImg>> keep;
    for ( auto& img : _imgs ) {
      if ( img.use_count() > 1 ) keep.push_back( std::move( img ) );
    }
    dropped = keep.size() != _imgs.size();
    _imgs = std::move( keep );
  }
  if ( _imgs.size() == before ) return;
  _idx.clear();
  for ( std::size_t i = 0; i < _imgs.size(); ++i ) _idx[_imgs[i].get()] = i;
  _entries.clear();
  _dirty = true;
}

}  // namespace gfx
