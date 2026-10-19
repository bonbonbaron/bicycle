#include "v/sdl/UiFont.h"
#include "data/Pack.h"
// stb_truetype's own allocations go to the GRAPHIC pool too.
#define STBTT_malloc( x, u ) ( (void)( u ), mem::memAdd( x, mem::GRAPHIC ) )
#define STBTT_free( x, u ) ( (void)( u ), ( x ) ? mem::memDel( x ) : (void)0 )
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#if defined( __GNUC__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "third_party/stb_truetype.h"
#if defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif

namespace gfx {

struct UiFont::Impl {
  stbtt_fontinfo info{};
  int ascent{}, descent{}, lineGap{};
};

UiFont::UiFont() : _impl( std::make_unique<Impl>() ) {}
UiFont::~UiFont() = default;

auto UiFont::load( const std::string& path ) -> bool {
  auto bytes = assets::read( path, mem::GRAPHIC );  // the embedded pack in release, the file in dev
  if ( !bytes ) return false;
  _bytes = std::move( *bytes );
  const auto* data = reinterpret_cast<const unsigned char*>( _bytes.data() );
  if ( !stbtt_InitFont( &_impl->info, data, stbtt_GetFontOffsetForIndex( data, 0 ) ) ) return false;
  stbtt_GetFontVMetrics( &_impl->info, &_impl->ascent, &_impl->descent, &_impl->lineGap );
  _cache.clear();
  _ok = true;
  return true;
}

auto UiFont::ascent( int px ) const -> float {
  return _impl->ascent * stbtt_ScaleForPixelHeight( &_impl->info, static_cast<float>( px ) );
}

auto UiFont::lineHeight( int px ) const -> float {
  return ( _impl->ascent - _impl->descent + _impl->lineGap ) * stbtt_ScaleForPixelHeight( &_impl->info, static_cast<float>( px ) );
}

auto UiFont::kern( uint32_t a, uint32_t b, int px ) const -> float {
  return stbtt_GetCodepointKernAdvance( &_impl->info, static_cast<int>( a ), static_cast<int>( b ) ) *
         stbtt_ScaleForPixelHeight( &_impl->info, static_cast<float>( px ) );
}

auto UiFont::glyph( uint32_t cp, int px ) -> const Glyph& {
  const uint64_t key = ( static_cast<uint64_t>( px ) << 32 ) | cp;
  if ( auto it = _cache.find( key ); it != _cache.end() ) return it->second;
  Glyph g;
  const float scale = stbtt_ScaleForPixelHeight( &_impl->info, static_cast<float>( px ) );
  int adv{}, lsb{};
  stbtt_GetCodepointHMetrics( &_impl->info, static_cast<int>( cp ), &adv, &lsb );
  g.advance = adv * scale;
  int x0, y0, x1, y1;
  stbtt_GetCodepointBitmapBox( &_impl->info, static_cast<int>( cp ), scale, scale, &x0, &y0, &x1, &y1 );
  g.w = x1 - x0;
  g.h = y1 - y0;
  g.xoff = x0;
  g.yoff = y0;
  if ( g.w > 0 && g.h > 0 ) {
    g.coverage.resize( static_cast<std::size_t>( g.w ) * g.h );
    stbtt_MakeCodepointBitmap( &_impl->info, g.coverage.data(), g.w, g.h, g.w, scale, scale, static_cast<int>( cp ) );
  }
  return _cache.emplace( key, std::move( g ) ).first->second;
}

auto UiFont::measure( const std::string& s, int px ) -> float {
  float w = 0;
  uint32_t prev = 0;
  for ( std::size_t i = 0; i < s.size(); ) {
    const uint32_t cp = nextCodepoint( s, i );
    if ( prev ) w += kern( prev, cp, px );
    w += glyph( cp, px ).advance;
    prev = cp;
  }
  return w;
}

auto nextCodepoint( const std::string& s, std::size_t& i ) -> uint32_t {
  const auto c = static_cast<unsigned char>( s[i++] );
  if ( c < 0x80 ) return c;
  int n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : -1;
  if ( n < 0 ) return '?';
  uint32_t cp = c & ( 0x3F >> n );
  for ( ; n > 0; --n ) {
    if ( i >= s.size() || ( static_cast<unsigned char>( s[i] ) & 0xC0 ) != 0x80 ) return '?';
    cp = ( cp << 6 ) | ( static_cast<unsigned char>( s[i++] ) & 0x3F );
  }
  return cp;
}

}  // namespace gfx
