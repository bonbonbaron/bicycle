// Shared by the 2D and 3D renderers: SDL3 window and keyboard, and a software framebuffer drawn in cells.
#include "v/sdl/PixelRenderer.h"
#include "v/sdl/Font8x8.h"
#include "v/common/Image.h"
#include "v/common/ColorPalette.h"
#include "c/Input.h"
#include "c/Trigger.h"
#include "m/World.h"
#include "m/Box.h"
#include "m/Rect.h"
#include "Constants.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace gfx {

auto colorToArgb( int color ) -> uint32_t {
  switch ( color ) {
    case BLACK:   return 0xFF000000u;
    case RED:     return 0xFFE04040u;
    case GREEN:   return 0xFF40C040u;
    case YELLOW:  return 0xFFE0D040u;
    case BLUE:    return 0xFF4060E0u;
    case MAGENTA: return 0xFFC050C0u;
    case CYAN:    return 0xFF40C0C0u;
    default:      return 0xFFE8E8E8u;
  }
}


static auto rgbaToArgb( uint32_t rgba ) -> uint32_t {  // BImg pixels: R in the low byte
  const uint32_t r = rgba & 0xFF, g = ( rgba >> 8 ) & 0xFF, b = ( rgba >> 16 ) & 0xFF, a = rgba >> 24;
  return ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
}

static auto wrapIdx( int i, int n ) -> int { return ( ( i % n ) + n ) % n; }

static auto envInt( const char* name, int def ) -> int {
  const char* v = std::getenv( name );
  return v && *v ? std::atoi( v ) : def;
}

auto PixelRenderer::windowFlags() const -> uint64_t {
  return SDL_WINDOW_RESIZABLE;
}

void PixelRenderer::init( const std::string& gameName ) {
  if ( !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) ) {
    std::fprintf( stderr, "SDL_Init failed: %s\n", SDL_GetError() );
    std::exit( 1 );
  }
  // Window size in screen pixels: BICYCLE_WINDOW_SIZE=WxH (default 1280x720, i.e. 160x45 cells).
  int w = 1280, h = 720;
  if ( const char* s = std::getenv( "BICYCLE_WINDOW_SIZE" ) ) {
    std::sscanf( s, "%dx%d", &w, &h );
  }
  _gameName = gameName;
  beforeWindowCreated();
  _window = SDL_CreateWindow( ( "bicycle - " + gameName ).c_str(), w, h, windowFlags() );
  if ( !_window ) {
    std::fprintf( stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError() );
    std::exit( 1 );
  }
  SDL_GetWindowSizeInPixels( _window, &_fbW, &_fbH );
  _fb.assign( static_cast<size_t>( _fbW ) * _fbH, frameClearColor() );
  onWindowCreated();
  _up = true;
}

void PixelRenderer::shutdown() {
  if ( !_up ) return;
  _up = false;
  onWindowDestroyed();
  SDL_DestroyWindow( _window );
  _window = nullptr;
  SDL_Quit();
}

auto PixelRenderer::checkResize() -> bool {
  if ( !_resized ) return false;
  _resized = false;
  int w{}, h{};
  SDL_GetWindowSizeInPixels( _window, &w, &h );
  if ( w == _fbW && h == _fbH ) return false;
  _fbW = std::max( w, CELL_W );
  _fbH = std::max( h, CELL_H );
  _fb.assign( static_cast<size_t>( _fbW ) * _fbH, frameClearColor() );
  onResized();
  return true;
}

static auto sdlKeyToLogical( SDL_Keycode k ) -> LogicalKey {
  if ( k >= SDLK_A && k <= SDLK_Z ) return static_cast<LogicalKey>( static_cast<int>( LogicalKey::A ) + ( k - SDLK_A ) );
  if ( k >= SDLK_1 && k <= SDLK_9 ) return static_cast<LogicalKey>( static_cast<int>( LogicalKey::Key1 ) + ( k - SDLK_1 ) );
  switch ( k ) {
    case SDLK_0: return LogicalKey::Key0;
    case SDLK_SPACE: return LogicalKey::Space;
    case SDLK_RETURN: case SDLK_KP_ENTER: return LogicalKey::Enter;
    case SDLK_ESCAPE: return LogicalKey::Escape;
    case SDLK_TAB: return LogicalKey::Tab;
    // Arrows drive the same keys as WASD.
    case SDLK_UP: return LogicalKey::W;
    case SDLK_LEFT: return LogicalKey::A;
    case SDLK_DOWN: return LogicalKey::S;
    case SDLK_RIGHT: return LogicalKey::D;
    default: return LogicalKey::COUNT;
  }
}

void PixelRenderer::handleEvent( const SDL_Event& e ) {
  switch ( e.type ) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      _quit = true;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      _resized = true;
      break;
    case SDL_EVENT_KEY_DOWN: {
      // Like the terminal: each press (and each auto-repeat while held) is one input event.
      const auto lkey = sdlKeyToLogical( e.key.key );
      if ( lkey != LogicalKey::COUNT ) {
        Input in{};
        in.currKeysPressed.set( static_cast<unsigned>( lkey ) );
        in.lastPressed = lkey;
        Trigger::sendInput( in );
      }
      break;
    }
    default:
      break;
  }
}

void PixelRenderer::pollInput() {
  SDL_Event e;
  while ( SDL_PollEvent( &e ) ) {
    handleEvent( e );
  }
}

// --- windows --------------------------------------------------------------------------------------------------------

#define WIN( h ) ( static_cast<Win*>( h ) )

auto PixelRenderer::createWindow( int x, int y, int w, int h ) -> WinHandle {
  _wins.push_back( std::make_unique<Win>( Win{ x, y, w, h } ) );
  return _wins.back().get();
}

void PixelRenderer::destroyWindow( WinHandle win ) {
  _wins.erase( std::remove_if( _wins.begin(), _wins.end(), [win]( const auto& p ) { return p.get() == win; } ), _wins.end() );
}

void PixelRenderer::moveWindow( WinHandle win, int x, int y ) {
  if ( win ) { WIN( win )->x = x; WIN( win )->y = y; }
}

void PixelRenderer::resizeWindow( WinHandle win, int w, int h ) {
  if ( win ) { WIN( win )->w = w; WIN( win )->h = h; }
}

auto PixelRenderer::windowX( WinHandle win ) const -> int { return win ? WIN( win )->x : 0; }
auto PixelRenderer::windowY( WinHandle win ) const -> int { return win ? WIN( win )->y : 0; }

void PixelRenderer::fillRect( const Win* win, int x, int y, int w, int h, uint32_t argb ) {
  if ( ( argb >> 24 ) == 0 ) return;
  int x0 = x, y0 = y, x1 = x + w, y1 = y + h;
  if ( win ) {
    x0 = std::max( x0, win->x * CELL_W );
    y0 = std::max( y0, win->y * CELL_H );
    x1 = std::min( x1, ( win->x + win->w ) * CELL_W );
    y1 = std::min( y1, ( win->y + win->h ) * CELL_H );
  }
  x0 = std::max( x0, 0 ); y0 = std::max( y0, 0 );
  x1 = std::min( x1, _fbW ); y1 = std::min( y1, _fbH );
  if ( x0 >= x1 || y0 >= y1 ) return;
  for ( int yy = y0; yy < y1; ++yy ) {
    std::fill( _fb.begin() + static_cast<size_t>( yy ) * _fbW + x0, _fb.begin() + static_cast<size_t>( yy ) * _fbW + x1, argb );
  }
}

void PixelRenderer::glyph( const Win* win, int px, int py, unsigned char c, uint32_t fg, uint32_t bg ) {
  fillRect( win, px, py, CELL_W, CELL_H, bg );
  const uint8_t* rows = FONT8X8[c < 128 ? c : '?'];
  for ( int r = 0; r < 8; ++r ) {
    for ( int b = 0; b < 8; ++b ) {
      if ( rows[r] & ( 1u << b ) ) {
        fillRect( win, px + b, py + r * 2 + 1, 1, 2, fg );  // 8x8 glyph drawn 8x16, nudged down a pixel
      }
    }
  }
}

void PixelRenderer::clearWindow( WinHandle win ) {
  Win* w = WIN( win );
  if ( !w ) return;
  fillRect( w, w->x * CELL_W, w->y * CELL_H, w->w * CELL_W, w->h * CELL_H, 0xFF101018u );
}

void PixelRenderer::drawBorder( WinHandle win ) {
  Win* w = WIN( win );
  if ( !w || ( w->attr & ATTR_INVISIBLE ) ) return;
  // A double line through the middle of the edge cells, like curses' box().
  const int x0 = w->x * CELL_W + CELL_W / 2 - 1, y0 = w->y * CELL_H + CELL_H / 2 - 1;
  const int x1 = ( w->x + w->w - 1 ) * CELL_W + CELL_W / 2 - 1, y1 = ( w->y + w->h - 1 ) * CELL_H + CELL_H / 2 - 1;
  const uint32_t c = 0xFFE8E8E8u;
  fillRect( w, x0, y0, x1 - x0 + 2, 2, c );
  fillRect( w, x0, y1, x1 - x0 + 2, 2, c );
  fillRect( w, x0, y0, 2, y1 - y0 + 2, c );
  fillRect( w, x1, y0, 2, y1 - y0 + 2, c );
}

void PixelRenderer::text( WinHandle win, int y, int x, const std::string& s ) {
  Win* w = WIN( win );
  if ( !w || ( w->attr & ATTR_INVISIBLE ) ) return;
  const bool standout = w->attr & ATTR_STANDOUT;
  const uint32_t fg = standout ? 0xFF101018u : 0xFFE8E8E8u;
  const uint32_t bg = standout ? 0xFFE8E8E8u : 0x00000000u;
  for ( size_t i = 0; i < s.size(); ++i ) {
    const unsigned char c = s[i];
    if ( c == ' ' && !standout ) continue;
    glyph( w, ( w->x + x + static_cast<int>( i ) ) * CELL_W, ( w->y + y ) * CELL_H, c, fg, bg );
  }
}

void PixelRenderer::setAttr( WinHandle win, int attr, bool on ) {
  if ( !win ) return;
  if ( on ) WIN( win )->attr |= attr;
  else WIN( win )->attr &= ~attr;
}

// BICYCLE_SCREENSHOT=<file.bmp> saves frame BICYCLE_SCREENSHOT_FRAME (default 90); BICYCLE_EXIT_AFTER_SCREENSHOT=1
// then quits. For headless runs (Xvfb) and docs.
auto PixelRenderer::screenshotDue() const -> bool {
  static const char* path = std::getenv( "BICYCLE_SCREENSHOT" );
  static const int when = envInt( "BICYCLE_SCREENSHOT_FRAME", 90 );
  return path && *path && _frame == when;
}

void PixelRenderer::saveScreenshotIfDue() {
  if ( !screenshotDue() ) return;
  const char* path = std::getenv( "BICYCLE_SCREENSHOT" );
  std::vector<uint32_t> shot;
  captureFrame( shot );
  if ( shot.size() != _fb.size() ) return;
  SDL_Surface* s = SDL_CreateSurfaceFrom( _fbW, _fbH, SDL_PIXELFORMAT_ARGB8888, shot.data(), _fbW * 4 );
  if ( s ) {
    if ( !SDL_SaveBMP( s, path ) ) std::fprintf( stderr, "screenshot failed: %s\n", SDL_GetError() );
    SDL_DestroySurface( s );
  }
  if ( envInt( "BICYCLE_EXIT_AFTER_SCREENSHOT", 0 ) ) _quit = true;
}

void PixelRenderer::endFrame() {
  ++_frame;
  presentFramebuffer();
  saveScreenshotIfDue();
  std::fill( _fb.begin(), _fb.end(), frameClearColor() );
}

// --- images ---------------------------------------------------------------------------------------------------------

// Text images (no pixels) in pixel modes: each non-space character as a glyph in the image's color.
void PixelRenderer::drawTextImage( const Win* w, const Image& img, int srcX, int srcY, int srcW, int srcH, int dstX, int dstY ) {
  const uint32_t fg = colorToArgb( img.color );
  for ( int r = 0; r < srcH; ++r ) {
    const int iy = srcY + r;
    if ( iy < 0 || iy >= static_cast<int>( img.lineLimits.size() ) ) continue;
    const auto& lims = img.lineLimits[iy];
    for ( int c = 0; c < srcW; ++c ) {
      const int ix = srcX + c;
      if ( ix < 0 || ix >= lims.len ) continue;
      const unsigned char ch = img.string[lims.start + ix];
      if ( ch != ' ' ) glyph( w, ( w->x + dstX + c ) * CELL_W, ( w->y + dstY + r ) * CELL_H, ch, fg, 0 );
    }
  }
}

void PixelRenderer::drawLayer( WinHandle win, const Image& img, int offX, int offY, bool wrap, int winW, int winH ) {
  Win* w = WIN( win );
  if ( !w ) return;
  if ( !img.pixels ) {  // text layer: same cell mapping as ASCII, glyph by glyph
    const int imgH = img.lineLimits.size();
    const int imgW = img.size.w >> FIXEDPT_DEC_BITS;
    if ( imgH == 0 || imgW == 0 ) return;
    for ( int r = 0; r < winH; ++r ) {
      int iy = r + offY;
      if ( wrap ) iy = wrapIdx( iy, imgH );
      for ( int c = 0; c < winW; ++c ) {
        int ix = c + offX;
        if ( wrap ) ix = wrapIdx( ix, imgW );
        drawTextImage( w, img, ix, iy, 1, 1, c, r );
      }
    }
    return;
  }
  // Window cell (r, c) shows image cell (r + offY, c + offX); an image pixel is 2 cells wide, 1 tall.
  const BImg& p = *img.pixels;
  const int ox = w->x * CELL_W, oy = w->y * CELL_H;
  const int px0 = offX >= 0 ? offX / 2 : -( ( -offX + 1 ) / 2 );  // floor( offX / 2 )
  const int px1 = ( offX + winW + 1 ) / 2 + 1;
  for ( int r = 0; r < winH; ++r ) {
    int iy = r + offY;
    if ( wrap ) iy = wrapIdx( iy, p.h );
    else if ( iy < 0 || iy >= p.h ) continue;
    for ( int px = px0; px < px1; ++px ) {
      int ix = px;
      if ( wrap ) ix = wrapIdx( ix, p.w );
      else if ( ix < 0 || ix >= p.w ) continue;
      const uint32_t argb = rgbaToArgb( p.pixel( ix, iy ) );
      fillRect( w, ox + ( 2 * px - offX ) * CELL_W, oy + r * CELL_H, 2 * CELL_W, CELL_H, argb );
    }
  }
}

void PixelRenderer::drawEntity( WinHandle win, Entity entity, const Box& camBox ) {
  Win* w = WIN( win );
  if ( !w ) return;
  const auto& box = World::get<Box>( entity );
  const auto& img = World::get<Image>( entity );
  const auto& src = World::get<Rect>( entity );
  const bool hasSrc = src.w > 0;
  const int ex = ( box.pos.x - camBox.pos.x ) >> FIXEDPT_DEC_BITS;  // entity's top-left in window cells
  const int ey = ( box.pos.y - camBox.pos.y ) >> FIXEDPT_DEC_BITS;
  const int sx = hasSrc ? src.x : 0, sy = hasSrc ? src.y : 0;
  const int sw = hasSrc ? src.w : img.size.w >> FIXEDPT_DEC_BITS;
  const int sh = hasSrc ? src.h : img.size.h >> FIXEDPT_DEC_BITS;
  if ( !img.pixels ) {
    drawTextImage( w, img, sx, sy, sw, sh, ex, ey );
    return;
  }
  const BImg& p = *img.pixels;
  const int ox = w->x * CELL_W, oy = w->y * CELL_H;
  for ( int r = 0; r < sh; ++r ) {
    const int iy = sy + r;
    if ( iy < 0 || iy >= p.h ) continue;
    for ( int c = 0; c < sw / 2; ++c ) {
      const int ix = sx / 2 + c;
      if ( ix < 0 || ix >= p.w ) continue;
      fillRect( w, ox + ( ex + 2 * c ) * CELL_W, oy + ( ey + r ) * CELL_H, 2 * CELL_W, CELL_H, rgbaToArgb( p.pixel( ix, iy ) ) );
    }
  }
}

}  // namespace gfx
