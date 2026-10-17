#include "c/StartupStats.h"
// Shared by the 2D and 3D renderers: SDL3 window and keyboard, and a software framebuffer drawn in cells.
#include "v/sdl/PixelRenderer.h"
#include "v/common/Capture.h"
#include "v/sdl/Font8x8.h"
#include "v/common/Image.h"
#include "v/common/ColorPalette.h"
#include "c/Input.h"
#include "c/InputState.h"
#include "c/Trigger.h"
#include "m/World.h"
#include "m/Box.h"
#include "m/Rect.h"
#include "Constants.h"
#include "data/BImg.h"
#include <cmath>
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
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
#if defined( BICYCLE_RELEASE ) && defined( __linux__ )
  // Release builds keep the OS libraries' own file traffic down too (unless the user set these): no mesa shader
  // cache in ~/.cache (the shaders are compiled from the strings in the executable each run; that takes
  // milliseconds).
  setenv( "MESA_SHADER_CACHE_DISABLE", "true", 0 );
#endif
  if ( !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) ) {
    std::fprintf( stderr, "SDL_Init failed: %s\n", SDL_GetError() );
    std::exit( 1 );
  }
  // Window size in screen pixels: BICYCLE_WINDOW_SIZE=WxH (default 1280x720, i.e. 160x45 cells).
  int w = 1280, h = 720;
  if ( const char* s = std::getenv( "BICYCLE_WINDOW_SIZE" ) ) {
    std::sscanf( s, "%dx%d", &w, &h );
  }
  startup::mark( "  SDL_Init (video)" );
  _gameName = gameName;
  // UI text: fonts/ui.ttf (a game can ship its own as <game>/fonts/ui.ttf).
  for ( const std::string dir : { "./" + gameName + "/fonts/", std::string( "./fonts/" ) } ) {
    if ( _font.load( dir + "ui.ttf" ) ) {
      _fontBold.load( dir + "ui-bold.ttf" );
      break;
    }
  }
  if ( !_font.loaded() ) std::fprintf( stderr, "bicycle: no fonts/ui.ttf; UI text falls back to the bitmap font\n" );
  startup::mark( "  UI fonts" );
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
    case SDLK_LSHIFT: return LogicalKey::LeftShift;
    case SDLK_RSHIFT: return LogicalKey::RightShift;
    case SDLK_LCTRL: return LogicalKey::LeftCtrl;
    case SDLK_RCTRL: return LogicalKey::RightCtrl;
    case SDLK_LALT: return LogicalKey::LeftAlt;
    case SDLK_RALT: return LogicalKey::RightAlt;
    // Arrows drive the same keys as WASD.
    case SDLK_UP: return LogicalKey::W;
    case SDLK_LEFT: return LogicalKey::A;
    case SDLK_DOWN: return LogicalKey::S;
    case SDLK_RIGHT: return LogicalKey::D;
    default: return LogicalKey::COUNT;
  }
}

// Several physical sources can drive one logical key (W and the up arrow, the left stick): a key is pressed when its
// first source goes down and released when its last one comes up, so games see one clean press/release pair.
static std::array<uint8_t, static_cast<size_t>( LogicalKey::COUNT )> sourcesDown{};
static void keyEdge( LogicalKey k, bool down ) {
  auto& n = sourcesDown[static_cast<size_t>( k )];
  if ( down ? n++ != 0 : ( n == 0 || --n != 0 ) ) return;
  Input in{};
  in.lastPressed = k;
  if ( down ) in.currKeysPressed.set( static_cast<unsigned>( k ) );
  Trigger::sendInput( in, down );
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
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
      // One press when a key goes down, one release when it comes up (auto-repeats are not new presses).
      if ( e.key.repeat ) break;
      const auto lkey = sdlKeyToLogical( e.key.key );
      if ( lkey != LogicalKey::COUNT ) keyEdge( lkey, e.type == SDL_EVENT_KEY_DOWN );
      break;
    }
    case SDL_EVENT_MOUSE_MOTION:
      inputState().mouseDx += e.motion.xrel;
      inputState().mouseDy += e.motion.yrel;
      // Mouse look only while the mouse is captured: an uncaptured pointer is just passing over the window.
      if ( inputState().mouseCaptured ) Trigger::sendLook( e.motion.xrel, e.motion.yrel );
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
      const int b = e.button.button == SDL_BUTTON_LEFT ? 0 : e.button.button == SDL_BUTTON_RIGHT ? 1 : e.button.button == SDL_BUTTON_MIDDLE ? 2 : -1;
      if ( b < 0 ) break;
      const bool down = e.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
      if ( down ) {
        inputState().mouseButtons |= 1u << b;
        inputState().mousePressed |= 1u << b;
      }
      else inputState().mouseButtons &= ~( 1u << b );
      keyEdge( static_cast<LogicalKey>( static_cast<int>( LogicalKey::MouseLeft ) + ( b == 0 ? 0 : b == 1 ? 1 : 2 ) ), down );
      break;
    }
    case SDL_EVENT_MOUSE_WHEEL:
      inputState().wheel += e.wheel.y;
      break;
    case SDL_EVENT_GAMEPAD_ADDED:
      if ( !_pad ) _pad = SDL_OpenGamepad( e.gdevice.which );
      break;
    case SDL_EVENT_GAMEPAD_REMOVED:
      if ( _pad && SDL_GetGamepadID( static_cast<SDL_Gamepad*>( _pad ) ) == e.gdevice.which ) {
        SDL_CloseGamepad( static_cast<SDL_Gamepad*>( _pad ) );
        _pad = nullptr;
      }
      break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
      if ( inputState().mouseCaptured ) setMouseCapture( false );
      break;
    default:
      break;
  }
}

void PixelRenderer::setMouseCapture( bool on ) {
  if ( _window ) SDL_SetWindowRelativeMouseMode( _window, on );
  inputState().mouseCaptured = on;
}

void PixelRenderer::enableGamepad() {
  if ( _padInit ) return;
  _padInit = SDL_InitSubSystem( SDL_INIT_GAMEPAD );
}

void PixelRenderer::pollInput() {
  auto& in = inputState();
  const Bitset before = in.held;
  const unsigned padBefore = in.padButtons;
  in.beginFrame();
  SDL_Event e;
  while ( SDL_PollEvent( &e ) ) {
    handleEvent( e );
  }
  // Held keys straight from SDL's keyboard state (the events above only report presses).
  int n{};
  const bool* ks = SDL_GetKeyboardState( &n );
  in.held.reset();
  auto hold = [&]( SDL_Scancode sc, LogicalKey k ) { if ( sc < n && ks[sc] ) in.held.set( static_cast<unsigned>( k ) ); };
  for ( int i = 0; i < 26; ++i ) hold( static_cast<SDL_Scancode>( SDL_SCANCODE_A + i ), static_cast<LogicalKey>( static_cast<int>( LogicalKey::A ) + i ) );
  for ( int i = 0; i < 9; ++i ) hold( static_cast<SDL_Scancode>( SDL_SCANCODE_1 + i ), static_cast<LogicalKey>( static_cast<int>( LogicalKey::Key1 ) + i ) );
  hold( SDL_SCANCODE_0, LogicalKey::Key0 );
  hold( SDL_SCANCODE_SPACE, LogicalKey::Space );
  hold( SDL_SCANCODE_RETURN, LogicalKey::Enter );
  hold( SDL_SCANCODE_ESCAPE, LogicalKey::Escape );
  hold( SDL_SCANCODE_TAB, LogicalKey::Tab );
  hold( SDL_SCANCODE_LSHIFT, LogicalKey::LeftShift );
  hold( SDL_SCANCODE_RSHIFT, LogicalKey::RightShift );
  hold( SDL_SCANCODE_LCTRL, LogicalKey::LeftCtrl );
  hold( SDL_SCANCODE_RCTRL, LogicalKey::RightCtrl );
  hold( SDL_SCANCODE_UP, LogicalKey::W );  // arrows = WASD, as for presses
  hold( SDL_SCANCODE_LEFT, LogicalKey::A );
  hold( SDL_SCANCODE_DOWN, LogicalKey::S );
  hold( SDL_SCANCODE_RIGHT, LogicalKey::D );
  in.pressed = in.held & ~before;
  in.padConnected = _pad != nullptr;
  in.padButtons = 0;
  if ( auto* pad = static_cast<SDL_Gamepad*>( _pad ) ) {
    auto axis = [&]( SDL_GamepadAxis a ) { return SDL_GetGamepadAxis( pad, a ) / 32767.0f; };
    in.axes = { axis( SDL_GAMEPAD_AXIS_LEFTX ), axis( SDL_GAMEPAD_AXIS_LEFTY ), axis( SDL_GAMEPAD_AXIS_RIGHTX ),
                axis( SDL_GAMEPAD_AXIS_RIGHTY ), axis( SDL_GAMEPAD_AXIS_LEFT_TRIGGER ), axis( SDL_GAMEPAD_AXIS_RIGHT_TRIGGER ) };
    for ( int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT && b < 32; ++b ) {
      if ( SDL_GetGamepadButton( pad, static_cast<SDL_GamepadButton>( b ) ) ) in.padButtons |= 1u << b;
    }
  }
  else in.axes = {};
  in.padPressed = in.padButtons & ~padBefore;
  // The gamepad as events too: the left stick is WASD, A is Space, the right trigger or bumper is the left mouse
  // button, the left stick's click is Shift, and the right stick is mouse look (600 px/s at full tilt).
  {
    static Bitset padKeys{};
    Bitset now{};
    auto key = []( LogicalKey k ) { return static_cast<unsigned>( k ); };
    const auto& a = in.axes;
    if ( a[1] < -0.5f ) now.set( key( LogicalKey::W ) );
    if ( a[1] > 0.5f ) now.set( key( LogicalKey::S ) );
    if ( a[0] < -0.5f ) now.set( key( LogicalKey::A ) );
    if ( a[0] > 0.5f ) now.set( key( LogicalKey::D ) );
    if ( in.padButtons & ( 1u << SDL_GAMEPAD_BUTTON_SOUTH ) ) now.set( key( LogicalKey::Space ) );
    if ( a[5] > 0.5f || ( in.padButtons & ( 1u << SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER ) ) ) now.set( key( LogicalKey::MouseLeft ) );
    if ( in.padButtons & ( 1u << SDL_GAMEPAD_BUTTON_LEFT_STICK ) ) now.set( key( LogicalKey::LeftShift ) );
    for ( unsigned k = 1; k < static_cast<unsigned>( LogicalKey::COUNT ); ++k ) {
      if ( now.test( k ) != padKeys.test( k ) ) keyEdge( static_cast<LogicalKey>( k ), now.test( k ) );
    }
    padKeys = now;
    constexpr float DEAD = 0.2f, STICK_LOOK = 600.f * static_cast<float>( Constants::MILLISECONDS_PER_FRAME / 1000.0 );
    const float rx = std::abs( a[2] ) > DEAD ? a[2] : 0, ry = std::abs( a[3] ) > DEAD ? a[3] : 0;
    if ( rx != 0 || ry != 0 ) Trigger::sendLook( rx * STICK_LOOK, ry * STICK_LOOK );
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

void PixelRenderer::setPanel( WinHandle win, Panel kind ) {
  if ( win ) WIN( win )->panel = kind;
}

// A scene's window is just its view (its layers cover it); dialogue and menu windows are panels drawn by the mode.
void PixelRenderer::clearWindow( WinHandle win ) {
  Win* w = WIN( win );
  if ( !w || ( w->attr & ATTR_INVISIBLE ) ) return;
  if ( w->panel == Panel::Scene ) {
    fillRect( w, w->x * CELL_W, w->y * CELL_H, w->w * CELL_W, w->h * CELL_H, 0xFF101018u );
    return;
  }
  drawPanelBody( *w );
}

void PixelRenderer::drawBorder( WinHandle win ) {
  Win* w = WIN( win );
  if ( !w || ( w->attr & ATTR_INVISIBLE ) || w->panel == Panel::Scene ) return;
  drawPanelFrame( *w );
}

// Text rows sit in cell rows (layout stays the same as ASCII), but the glyphs are proportional TrueType.
void PixelRenderer::text( WinHandle win, int y, int x, const std::string& s ) {
  Win* w = WIN( win );
  if ( !w || ( w->attr & ATTR_INVISIBLE ) ) return;
  // Panels get a few pixels more inset than a cell, so text clears the frame.
  const int px = ( w->x + x ) * CELL_W + ( w->panel == Panel::Scene ? 0 : 6 ), py = ( w->y + y ) * CELL_H;
  if ( w->attr & ATTR_STANDOUT ) {
    drawRowHighlight( *w, px - 4, py, measureString( s, UI_TEXT_PX ) + 8, CELL_H );
  }
  drawString( w, px, py, s, 0xFFF2EEE4u, UI_TEXT_PX );
}

// --- UI primitives --------------------------------------------------------------------------------------------------

void PixelRenderer::blendPixel( int x, int y, uint32_t argb, unsigned coverage ) {
  if ( x < 0 || y < 0 || x >= _fbW || y >= _fbH ) return;
  const unsigned sa = ( ( argb >> 24 ) * coverage + 127 ) / 255;
  if ( sa == 0 ) return;
  uint32_t& d = _fb[static_cast<size_t>( y ) * _fbW + x];
  if ( sa == 255 ) { d = ( argb & 0x00FFFFFFu ) | 0xFF000000u; return; }
  const unsigned da = d >> 24;
  const unsigned oa = sa + da * ( 255 - sa ) / 255;
  if ( oa == 0 ) return;
  auto ch = [&]( int sh ) -> uint32_t {
    const unsigned s = ( argb >> sh ) & 0xFF, dd = ( d >> sh ) & 0xFF;
    return ( s * sa + dd * da * ( 255 - sa ) / 255 ) / oa;
  };
  d = ( oa << 24 ) | ( ch( 16 ) << 16 ) | ( ch( 8 ) << 8 ) | ch( 0 );
}

void PixelRenderer::clip( const Win* w, int& x0, int& y0, int& x1, int& y1 ) const {
  if ( w ) {
    x0 = std::max( x0, w->x * CELL_W ); y0 = std::max( y0, w->y * CELL_H );
    x1 = std::min( x1, ( w->x + w->w ) * CELL_W ); y1 = std::min( y1, ( w->y + w->h ) * CELL_H );
  }
  x0 = std::max( x0, 0 ); y0 = std::max( y0, 0 );
  x1 = std::min( x1, _fbW ); y1 = std::min( y1, _fbH );
}

void PixelRenderer::blendRect( const Win* win, int x, int y, int w, int h, uint32_t argb ) {
  int x0 = x, y0 = y, x1 = x + w, y1 = y + h;
  clip( win, x0, y0, x1, y1 );
  for ( int yy = y0; yy < y1; ++yy )
    for ( int xx = x0; xx < x1; ++xx ) blendPixel( xx, yy, argb );
}

// Coverage of pixel (px, py) by a rounded rect, 0..255 (one sample per pixel with a 1-pixel soft edge).
static auto roundCoverage( float px, float py, float x, float y, float w, float h, float r ) -> float {
  const float cx = std::clamp( px, x + r, x + w - r ), cy = std::clamp( py, y + r, y + h - r );
  const float d = std::hypot( px - cx, py - cy ) - r;  // signed distance to the edge (negative inside)
  return std::clamp( 0.5f - d, 0.0f, 1.0f );
}

void PixelRenderer::roundRect( const Win* win, int x, int y, int w, int h, int radius, uint32_t argb, int thickness ) {
  int x0 = x, y0 = y, x1 = x + w, y1 = y + h;
  clip( win, x0, y0, x1, y1 );
  const float r = static_cast<float>( std::min( { radius, w / 2, h / 2 } ) );
  for ( int yy = y0; yy < y1; ++yy ) {
    for ( int xx = x0; xx < x1; ++xx ) {
      const float px = xx + 0.5f, py = yy + 0.5f;
      float c = roundCoverage( px, py, x, y, w, h, r );
      if ( thickness > 0 ) {
        const float t = static_cast<float>( thickness );
        c -= roundCoverage( px, py, x + t, y + t, w - 2 * t, h - 2 * t, std::max( 0.0f, r - t ) );
      }
      if ( c > 0 ) blendPixel( xx, yy, argb, static_cast<unsigned>( c * 255 + 0.5f ) );
    }
  }
}

static auto lerpArgb( uint32_t a, uint32_t b, float t ) -> uint32_t {
  uint32_t o = 0;
  for ( int sh = 0; sh < 32; sh += 8 ) {
    const float va = ( a >> sh ) & 0xFF, vb = ( b >> sh ) & 0xFF;
    o |= static_cast<uint32_t>( va + ( vb - va ) * t + 0.5f ) << sh;
  }
  return o;
}

void PixelRenderer::gradientRoundRect( const Win* win, int x, int y, int w, int h, int radius, uint32_t top, uint32_t bottom ) {
  int x0 = x, y0 = y, x1 = x + w, y1 = y + h;
  clip( win, x0, y0, x1, y1 );
  const float r = static_cast<float>( std::min( { radius, w / 2, h / 2 } ) );
  for ( int yy = y0; yy < y1; ++yy ) {
    const uint32_t c = lerpArgb( top, bottom, h > 1 ? static_cast<float>( yy - y ) / ( h - 1 ) : 0.0f );
    for ( int xx = x0; xx < x1; ++xx ) {
      const float cov = roundCoverage( xx + 0.5f, yy + 0.5f, x, y, w, h, r );
      if ( cov > 0 ) blendPixel( xx, yy, c, static_cast<unsigned>( cov * 255 + 0.5f ) );
    }
  }
}

void PixelRenderer::drawSprite( const Win* win, const BImg& img, int x, int y, int w, int h, float fraction ) {
  if ( img.w <= 0 || img.h <= 0 || w <= 0 || h <= 0 ) return;
  int x0 = x, y0 = y, x1 = x + static_cast<int>( w * std::clamp( fraction, 0.0f, 1.0f ) + 0.5f ), y1 = y + h;
  clip( win, x0, y0, x1, y1 );
  for ( int yy = y0; yy < y1; ++yy ) {
    const int iy = ( yy - y ) * img.h / h;
    for ( int xx = x0; xx < x1; ++xx ) {
      blendPixel( xx, yy, rgbaToArgb( img.pixel( ( xx - x ) * img.w / w, iy ) ) );
    }
  }
}

auto PixelRenderer::measureString( const std::string& s, int sizePx, bool bold ) -> int {
  UiFont& f = bold && _fontBold.loaded() ? _fontBold : _font;
  if ( !f.loaded() ) return static_cast<int>( s.size() ) * CELL_W;
  return static_cast<int>( std::ceil( f.measure( s, sizePx ) ) );
}

auto PixelRenderer::drawString( const Win* win, int px, int top, const std::string& s, uint32_t argb, int sizePx, bool bold ) -> int {
  UiFont& f = bold && _fontBold.loaded() ? _fontBold : _font;
  if ( !f.loaded() ) {
    for ( size_t i = 0; i < s.size(); ++i ) glyph( win, px + static_cast<int>( i ) * CELL_W, top, s[i], argb, 0 );
    return static_cast<int>( s.size() ) * CELL_W;
  }
  int cx0 = 0, cy0 = 0, cx1 = _fbW, cy1 = _fbH;
  clip( win, cx0, cy0, cx1, cy1 );
  // Center the line in a cell-row-sized box: baseline at top + (row height + ascent - descent) / 2.
  const float baseline = top + std::round( ( CELL_H * sizePx / UI_TEXT_PX - f.lineHeight( sizePx ) ) / 2 + f.ascent( sizePx ) );
  float pen = static_cast<float>( px );
  uint32_t prev = 0;
  for ( size_t i = 0; i < s.size(); ) {
    const uint32_t cp = nextCodepoint( s, i );
    if ( prev ) pen += f.kern( prev, cp, sizePx );
    const auto& g = f.glyph( cp, sizePx );
    const int gx = static_cast<int>( std::lround( pen ) ) + g.xoff, gy = static_cast<int>( baseline ) + g.yoff;
    for ( int r = 0; r < g.h; ++r ) {
      const int yy = gy + r;
      if ( yy < cy0 || yy >= cy1 ) continue;
      for ( int c = 0; c < g.w; ++c ) {
        const int xx = gx + c;
        if ( xx < cx0 || xx >= cx1 ) continue;
        if ( const unsigned cov = g.coverage[static_cast<size_t>( r ) * g.w + c] ) blendPixel( xx, yy, argb, cov );
      }
    }
    pen += g.advance;
    prev = cp;
  }
  return static_cast<int>( std::ceil( pen ) ) - px;
}

void PixelRenderer::setAttr( WinHandle win, int attr, bool on ) {
  if ( !win ) return;
  if ( on ) WIN( win )->attr |= attr;
  else WIN( win )->attr &= ~attr;
}

// BICYCLE_SCREENSHOT / BICYCLE_SCREENSHOT_FRAMES (include/v/common/Capture.h) save frames as .bmp;
// BICYCLE_EXIT_AFTER_SCREENSHOT=1 quits after the last one. For headless runs (Xvfb) and docs.
auto PixelRenderer::screenshotDue() const -> bool {
  return capture::pathFor( _frame ).has_value();
}

void PixelRenderer::saveScreenshotIfDue() {
#ifndef BICYCLE_RELEASE
  const auto path = capture::pathFor( _frame );
  if ( !path ) return;
  gfx::Pixels shot;
  captureFrame( shot );
  if ( shot.size() != _fb.size() ) return;
  SDL_Surface* s = SDL_CreateSurfaceFrom( _fbW, _fbH, SDL_PIXELFORMAT_ARGB8888, shot.data(), _fbW * 4 );
  if ( s ) {
    if ( !SDL_SaveBMP( s, path->c_str() ) ) std::fprintf( stderr, "screenshot failed: %s\n", SDL_GetError() );
    SDL_DestroySurface( s );
  }
  if ( envInt( "BICYCLE_EXIT_AFTER_SCREENSHOT", 0 ) && _frame >= capture::lastFrame() ) _quit = true;
#endif
}

// Scene transitions over the finished frame (setTransition). 2D: the framebuffer is the screen. 3D: it's the overlay
// composited over the GL world with alpha, so covering it with (alpha) black darkens the world under it too.
void PixelRenderer::applyTransition() {
  if ( _transKind == 0 || _transT <= 0.0f ) return;
  const float t = std::min( 1.0f, _transT );
  if ( _transKind == 2 ) {  // wipe: a black edge sweeping left to right
    const int edge = static_cast<int>( t * _fbW );
    for ( int y = 0; y < _fbH; ++y ) std::fill_n( _fb.begin() + static_cast<size_t>( y ) * _fbW, edge, 0xFF000000u );
    return;
  }
  if ( _transKind == 3 ) {  // dissolve: 8x8 blocks drop out in a fixed scattered order
    for ( int by = 0; by < _fbH; by += 8 ) {
      for ( int bx = 0; bx < _fbW; bx += 8 ) {
        uint32_t h = static_cast<uint32_t>( bx / 8 ) * 73856093u ^ static_cast<uint32_t>( by / 8 ) * 19349663u;
        h ^= h >> 13;
        h *= 0x5bd1e995u;
        h ^= h >> 15;
        if ( ( h & 0xFFFF ) / 65535.0f >= t ) continue;
        for ( int y = by; y < std::min( _fbH, by + 8 ); ++y )
          std::fill_n( _fb.begin() + static_cast<size_t>( y ) * _fbW + bx, std::min( 8, _fbW - bx ), 0xFF000000u );
      }
    }
    return;
  }
  // fade: scale the color down and the alpha up
  const unsigned k = static_cast<unsigned>( ( 1.0f - t ) * 256.0f );
  const unsigned addA = static_cast<unsigned>( t * 255.0f );
  for ( auto& p : _fb ) {
    const unsigned a = p >> 24;
    const unsigned r = ( ( p >> 16 ) & 0xFF ) * k >> 8, g = ( ( p >> 8 ) & 0xFF ) * k >> 8, b = ( p & 0xFF ) * k >> 8;
    const unsigned na = std::min( 255u, a + ( ( 255 - a ) * addA >> 8 ) + ( addA == 255 ? 1 : 0 ) );
    p = ( na << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
  }
}

void PixelRenderer::endFrame() {
  ++_frame;
  applyTransition();
  presentFramebuffer();
  saveScreenshotIfDue();
  std::fill( _fb.begin(), _fb.end(), frameClearColor() );
}

// --- images ---------------------------------------------------------------------------------------------------------

// Text images (ASCII art with no pixel version) in pixel modes: no character glyphs (that would be ASCII art in a
// pixel mode); each non-space cell becomes a solid block of the image's color, i.e. a silhouette at cell resolution.
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
      if ( ch != ' ' ) fillRect( w, ( w->x + dstX + c ) * CELL_W, ( w->y + dstY + r ) * CELL_H, CELL_W, CELL_H, fg );
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
  // Pixels come from the texture atlas (already ARGB).
  const BImg& p = *img.pixels;
  const auto at = _atlas.entry( img.pixels );
  // Smooth scrolling: the whole layer moves up/left by the camera's sub-cell remainder in screen pixels, so one
  // more row and pixel column are drawn to fill the edge. Every image pixel is still a whole solid rect of the
  // framebuffer (nearest, no filtering), so tiles stay crisp and can't open seams.
  const int ox = w->x * CELL_W - _subX, oy = w->y * CELL_H - _subY;
  const int px0 = offX >= 0 ? offX / 2 : -( ( -offX + 1 ) / 2 );  // floor( offX / 2 )
  const int px1 = ( offX + winW + 1 ) / 2 + 2;
  for ( int r = 0; r < winH + ( _subY ? 1 : 0 ); ++r ) {
    int iy = r + offY;
    if ( wrap ) iy = wrapIdx( iy, p.h );
    else if ( iy < 0 || iy >= p.h ) continue;
    for ( int px = px0; px < px1; ++px ) {
      int ix = px;
      if ( wrap ) ix = wrapIdx( ix, p.w );
      else if ( ix < 0 || ix >= p.w ) continue;
      const uint32_t argb = _atlas.at( at.x + ix, at.y + iy );
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
  // The camera's whole cell and its remainder, the same split the scene's layers get (Scene::render), so
  // sprites and the ground move together to the screen pixel.
  constexpr int FRAC = ( 1 << FIXEDPT_DEC_BITS ) - 1;
  const int camX = camBox.pos.x & ~FRAC, camY = camBox.pos.y & ~FRAC;
  const int ex = ( box.pos.x - camX ) >> FIXEDPT_DEC_BITS;  // entity's top-left in window cells (text images)
  const int ey = ( box.pos.y - camY ) >> FIXEDPT_DEC_BITS;
  const int exPx = ( ( box.pos.x - camX ) * CELL_W >> FIXEDPT_DEC_BITS ) - ( ( camBox.pos.x & FRAC ) * CELL_W >> FIXEDPT_DEC_BITS );
  const int eyPx = ( ( box.pos.y - camY ) * CELL_H >> FIXEDPT_DEC_BITS ) - ( ( camBox.pos.y & FRAC ) * CELL_H >> FIXEDPT_DEC_BITS );
  const int sx = hasSrc ? src.x : 0, sy = hasSrc ? src.y : 0;
  const int sw = hasSrc ? src.w : img.size.w >> FIXEDPT_DEC_BITS;
  const int sh = hasSrc ? src.h : img.size.h >> FIXEDPT_DEC_BITS;
  if ( !img.pixels ) {
    drawTextImage( w, img, sx, sy, sw, sh, ex, ey );
    return;
  }
  // The sprite's src rect is a sub-rect of its image's place in the atlas.
  const BImg& p = *img.pixels;
  const auto at = _atlas.entry( img.pixels );
  const int ox = w->x * CELL_W, oy = w->y * CELL_H;
  for ( int r = 0; r < sh; ++r ) {
    const int iy = sy + r;
    if ( iy < 0 || iy >= p.h ) continue;
    for ( int c = 0; c < sw / 2; ++c ) {
      const int ix = sx / 2 + c;
      if ( ix < 0 || ix >= p.w ) continue;
      fillRect( w, ox + exPx + 2 * c * CELL_W, oy + eyPx + r * CELL_H, 2 * CELL_W, CELL_H, _atlas.at( at.x + ix, at.y + iy ) );
    }
  }
}

}  // namespace gfx
