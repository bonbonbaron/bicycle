// ASCII mode: the terminal through curses. This is the engine's original renderer, moved behind gfx::Renderer.
#include "v/ascii/AsciiRenderer.h"
#include "v/common/Capture.h"
#include "curses_compat.h"
#include "c/SshInputListener.h"
#include "c/InputListener.h"
#include "m/World.h"
#include "m/Box.h"
#include "m/Rect.h"
#include "v/common/Image.h"
#include "v/common/ColorPalette.h"
#include "Constants.h"
#include <algorithm>
#include <chrono>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace gfx {

static auto W( WinHandle win ) -> WINDOW* {
  return static_cast<WINDOW*>( win );
}

void AsciiRenderer::init( const std::string& ) {
#ifdef BICYCLE_RELEASE
  setlocale( LC_CTYPE, "" );  // only the character set (UTF-8 for the box drawing): one locale file, not twelve
#else
  setlocale( LC_ALL, "" );
#endif
  initscr();
  start_color();
  cbreak();  // reads each character as they're typed instead of waiting for newline
  noecho();  // doesn't echo user's input back to them
  curs_set( 0 );
  // Output: the cursor's position doesn't matter (no moving it back after each update), hardware line/char
  // insert-delete may be used for scrolling diffs, and doupdate() never stops to poll stdin for typeahead (input
  // is read by SshInputListener, not curses). Input: getch() never blocks.
  leaveok( stdscr, TRUE );
  idlok( stdscr, TRUE );
  idcok( stdscr, TRUE );
  typeahead( -1 );
  nodelay( stdscr, TRUE );
  timeout( 0 );
  init_pair( BLACK, COLOR_BLACK, COLOR_BLACK );
  init_pair( RED, COLOR_RED, COLOR_BLACK );
  init_pair( GREEN, COLOR_GREEN, COLOR_BLACK );
  init_pair( YELLOW, COLOR_YELLOW, COLOR_BLACK );
  init_pair( BLUE, COLOR_BLUE, COLOR_BLACK );
  init_pair( MAGENTA, COLOR_MAGENTA, COLOR_BLACK );
  init_pair( CYAN, COLOR_CYAN, COLOR_BLACK );
  init_pair( WHITE, COLOR_WHITE, COLOR_BLACK );
  ::clear();
  ::refresh();
  _tooSmall = COLS < MIN_COLS || LINES < MIN_LINES;
  _up = true;
}

void AsciiRenderer::shutdown() {
  if ( _up ) {
    endwin();
    _up = false;
    // BICYCLE_FRAME_STATS=1: how long drawing a frame took (clearing the first window to doupdate() returning).
    if ( const char* s = std::getenv( "BICYCLE_FRAME_STATS" ); s && *s == '1' && _frames > 0 ) {
      std::fprintf( stderr, "bicycle ascii: %ld frames, render %.3f ms/frame avg, %.3f ms max\n", _frames,
                    _renderNs / 1e6 / _frames, _maxNs / 1e6 );
    }
  }
}

auto AsciiRenderer::cols() const -> int {
  return COLS;
}

auto AsciiRenderer::lines() const -> int {
  return LINES;
}

// curses reports a resize as KEY_RESIZE from getch(), which bicycle doesn't call (SshInputListener reads stdin
// itself), so poll for it once a frame instead: PDCurses' is_termresized() on Windows, the terminal size (what
// ncurses' SIGWINCH handler would read) elsewhere. Neither opens a file, so release builds stay I/O-free.
auto AsciiRenderer::checkResize() -> bool {
#ifdef _WIN32
  if ( !is_termresized() ) {
    return false;
  }
  resize_term( 0, 0 );  // PDCurses: adopt the console's new size
#else
  winsize ws{};
  if ( ioctl( STDOUT_FILENO, TIOCGWINSZ, &ws ) != 0 ) {
    return false;
  }
  const int rows = ws.ws_row;
  const int cols = ws.ws_col;
  if ( rows <= 0 || cols <= 0 || ( rows == LINES && cols == COLS ) ) {
    return false;
  }
  resize_term( rows, cols );  // ncurses: resizes stdscr, curscr and every window that would stick out
#endif
  _tooSmall = COLS < MIN_COLS || LINES < MIN_LINES;
  // Forget what curses thinks is on screen: the terminal may have reflowed or dropped it, so the next doupdate()
  // repaints every cell instead of diffing against stale contents.
  clearok( curscr, TRUE );
  ::clear();
  wnoutrefresh( stdscr );
  return true;
}

void AsciiRenderer::pollInput() {
  // SshInput::listen();
  auto input = InputListener::getInstance();
  input.listen();
}

// Each window is an off-screen buffer: frames werase() and redraw it, wnoutrefresh() copies it to curses' virtual
// screen, and the one doupdate() per frame sends only the cells that changed.
// curses refuses windows that stick out of the screen, so one asked for while the terminal is small is clipped to it
// (it's re-fit by Window::onTermResize when the terminal grows).
auto AsciiRenderer::createWindow( int x, int y, int w, int h ) -> WinHandle {
  w = std::clamp( w, 1, std::max( 1, COLS ) );
  h = std::clamp( h, 1, std::max( 1, LINES ) );
  x = std::clamp( x, 0, std::max( 0, COLS - w ) );
  y = std::clamp( y, 0, std::max( 0, LINES - h ) );
  WINDOW* win = newwin( h, w, y, x );
  if ( win ) {
    leaveok( win, TRUE );
    idlok( win, TRUE );
    idcok( win, TRUE );
    nodelay( win, TRUE );
  }
  return win;
}

void AsciiRenderer::destroyWindow( WinHandle win ) {
  if ( win ) {
    delwin( W( win ) );
  }
}

void AsciiRenderer::moveWindow( WinHandle win, int x, int y ) {
  mvwin( W( win ), y, x );
}

void AsciiRenderer::resizeWindow( WinHandle win, int w, int h ) {
  wresize( W( win ), h, w );
}

auto AsciiRenderer::windowX( WinHandle win ) const -> int {
  return getbegx( W( win ) );
}

auto AsciiRenderer::windowY( WinHandle win ) const -> int {
  return getbegy( W( win ) );
}

void AsciiRenderer::clearWindow( WinHandle win ) {
  if ( !_inFrame ) {
    _inFrame = true;
    _frameStart = std::chrono::steady_clock::now().time_since_epoch().count();
  }
  werase( W( win ) );
}

void AsciiRenderer::drawBorder( WinHandle win ) {
  ::box( W( win ), 0, 0 );
}

// ASCII's HUD is text. A bar: HP [#######---] 7/10 (any value above 0 shows at least one segment).
void AsciiRenderer::drawWidget( WinHandle win, int y, int x, Align align, const Widget& w ) {
  std::string s = w.text;
  if ( w.kind == Widget::BAR ) {
    const int filled = ( w.curr * w.len + w.max - 1 ) / w.max;
    s = ( w.text.empty() ? "[" : w.text + " [" ) + std::string( filled, '#' ) + std::string( w.len - filled, '-' ) + "] ";
    s += std::to_string( w.curr ) + "/" + std::to_string( w.max );
  }
  const int n = static_cast<int>( s.size() );
  if ( align == Align::End ) x = std::max( 0, x - n - 1 );
  else if ( align == Align::Center ) x = std::max( 0, x - n / 2 );
  text( win, y, x, s );
}

void AsciiRenderer::drawMenuCursor( WinHandle win, int y, int x, const std::string& asciiCursor ) {
  text( win, y, x, asciiCursor );
}

void AsciiRenderer::text( WinHandle win, int y, int x, const std::string& s ) {
  mvwaddnstr( W( win ), y, x, s.data(), static_cast<int>( s.size() ) );  // no printf formatting per string
}

void AsciiRenderer::setAttr( WinHandle win, int attr, bool on ) {
  int a{};
  if ( attr & ATTR_STANDOUT ) a |= A_STANDOUT;
  if ( attr & ATTR_INVISIBLE ) a |= A_INVIS;
  if ( on ) {
    wattron( W( win ), a );
  }
  else {
    wattroff( W( win ), a );
  }
}

// Scene transitions in a terminal: cells are blanked in a fixed scattered order (dissolve; also used for fade) or
// column by column (wipe), in every window as it's presented, so the dialogue and HUD dissolve with the world.
static auto dissolveRank( int x, int y ) -> float {
  uint32_t h = static_cast<uint32_t>( x ) * 73856093u ^ static_cast<uint32_t>( y ) * 19349663u;
  h ^= h >> 13;
  h *= 0x5bd1e995u;
  h ^= h >> 15;
  return ( h & 0xFFFF ) / 65535.0f;
}

void AsciiRenderer::presentWindow( WinHandle win ) {
  if ( _transKind != 0 && _transT > 0.0f ) {
    WINDOW* w = W( win );
    const int x0 = getbegx( w ), y0 = getbegy( w ), wh = getmaxy( w ), ww = getmaxx( w );
    const float t = std::min( 1.0f, _transT );
    for ( int y = 0; y < wh; ++y ) {
      for ( int x = 0; x < ww; ++x ) {
        const bool gone = _transKind == 2 ? x0 + x < static_cast<int>( t * COLS ) : dissolveRank( x0 + x, y0 + y ) < t;
        if ( gone ) mvwaddch( w, y, x, ' ' );
      }
    }
  }
  wnoutrefresh( W( win ) );
}

void AsciiRenderer::endFrame() {
  if ( _tooSmall ) {  // WindowManager drew nothing: say why, centered as far as it fits
    char msg[64];
    std::snprintf( msg, sizeof msg, "Terminal too small (%dx%d), need %dx%d", COLS, LINES, MIN_COLS, MIN_LINES );
    if ( static_cast<int>( std::strlen( msg ) ) > COLS ) {
      std::snprintf( msg, sizeof msg, "Too small" );
    }
    const int len = static_cast<int>( std::strlen( msg ) );
    werase( stdscr );
    mvwaddnstr( stdscr, LINES / 2, std::max( 0, ( COLS - len ) / 2 ), msg, COLS );
    wnoutrefresh( stdscr );
  }
  doupdate();  // displays results of the window painting
#ifndef BICYCLE_RELEASE
  // BICYCLE_SCREENSHOT(_FRAMES) (include/v/common/Capture.h): the screen's text.
  if ( auto path = capture::pathFor( _frames ) ) {
    if ( FILE* f = std::fopen( path->c_str(), "w" ) ) {
      std::string line( COLS + 1, '\0' );
      for ( int y = 0; y < LINES; ++y ) {
        mvwinnstr( curscr, y, 0, line.data(), COLS );
        std::fprintf( f, "%s\n", line.c_str() );
      }
      std::fclose( f );
    }
  }
#endif
  if ( _inFrame ) {
    const long long ns = std::chrono::steady_clock::now().time_since_epoch().count() - _frameStart;
    _renderNs += ns;
    _maxNs = std::max( _maxNs, ns );
    ++_frames;
    _inFrame = false;
  }
}

auto renderer() -> Renderer& {
  static AsciiRenderer r;
  return r;
}

}  // namespace gfx
