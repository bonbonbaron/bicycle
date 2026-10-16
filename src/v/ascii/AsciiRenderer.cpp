// ASCII mode: the terminal through curses. This is the engine's original renderer, moved behind gfx::Renderer.
#include "v/ascii/AsciiRenderer.h"
#include "curses_compat.h"
#include "c/SshInputListener.h"
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
  setlocale( LC_ALL, "" );
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

// ncurses only notices a resized terminal through getch(), which bicycle doesn't use, so check for it here.
auto AsciiRenderer::checkResize() -> bool {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO info{};
  if ( !GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE ), &info ) ) {
    return false;
  }
  const int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
  const int cols = info.srWindow.Right - info.srWindow.Left + 1;
#else
  winsize ws{};
  if ( ioctl( STDOUT_FILENO, TIOCGWINSZ, &ws ) != 0 ) {
    return false;
  }
  const int rows = ws.ws_row;
  const int cols = ws.ws_col;
#endif
  if ( rows <= 0 || cols <= 0 || ( rows == LINES && cols == COLS ) ) {
    return false;
  }
  resize_term( rows, cols );
  ::clear();
  wnoutrefresh( stdscr );
  return true;
}

void AsciiRenderer::pollInput() {
  SshInput::listen();
}

// Each window is an off-screen buffer: frames werase() and redraw it, wnoutrefresh() copies it to curses' virtual
// screen, and the one doupdate() per frame sends only the cells that changed.
auto AsciiRenderer::createWindow( int x, int y, int w, int h ) -> WinHandle {
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

// ASCII's bar is text: HP [#######---] 7/10 (any value above 0 shows at least one segment).
void AsciiRenderer::drawBar( WinHandle win, int y, int x, const BarSpec& bar ) {
  const int filled = ( bar.curr * bar.len + bar.max - 1 ) / bar.max;
  std::string s = bar.label.empty() ? "[" : bar.label + " [";
  s += std::string( filled, '#' ) + std::string( bar.len - filled, '-' ) + "] ";
  s += std::to_string( bar.curr ) + "/" + std::to_string( bar.max );
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

void AsciiRenderer::presentWindow( WinHandle win ) {
  wnoutrefresh( W( win ) );
}

void AsciiRenderer::endFrame() {
  doupdate();  // displays results of the window painting
  if ( _inFrame ) {
    const long long ns = std::chrono::steady_clock::now().time_since_epoch().count() - _frameStart;
    _renderNs += ns;
    _maxNs = std::max( _maxNs, ns );
    ++_frames;
    _inFrame = false;
  }
}

static auto wrapIdx( const int i, const int n ) -> int {
  return ( ( i % n ) + n ) % n;
}

// Spaces are transparent so layers behind show through.
void AsciiRenderer::drawLayer( WinHandle win, const Image& img, int offX, int offY, bool wrap, int w, int h ) {
  const int imgH = img.lineLimits.size();
  const int imgW = img.size.w >> FIXEDPT_DEC_BITS;
  if ( imgH == 0 || imgW == 0 ) {
    return;
  }
  std::string row( w, ' ' );
  for ( int r = 0; r < h; ++r ) {
    int iy = r + offY;
    if ( wrap ) {
      iy = wrapIdx( iy, imgH );
    }
    else if ( iy < 0 || iy >= imgH ) {
      continue;
    }
    const auto& lims = img.lineLimits.at( iy );
    for ( int c = 0; c < w; ++c ) {
      int ix = c + offX;
      if ( wrap ) {
        ix = wrapIdx( ix, imgW );
      }
      row[c] = ( ix >= 0 && ix < lims.len ) ? img.string[ lims.start + ix ] : ' ';
    }
    // Print runs of non-space characters only.
    for ( int c = 0; c < w; ) {
      if ( row[c] == ' ' ) {
        ++c;
        continue;
      }
      int end = c;
      while ( end < w && row[end] != ' ' ) {
        ++end;
      }
      text( win, r, c, row.substr( c, end - c ) );
      c = end;
    }
  }
}

// Prints the part of the entity's image (or its current animation frame) that the camera sees.
void AsciiRenderer::drawEntity( WinHandle win, Entity entity, const Box& camBox ) {
  const auto& entityBox = World::get<Box>( entity );
  const auto& entityImg = World::get<Image>( entity );
  const auto& srcRect = World::get<Rect>( entity );
  const bool hasSrcRect = srcRect.w > 0;
  auto croppedBox = camBox.crop( entityBox ) >> FIXEDPT_DEC_BITS;  // portion of entityBox in FOV
  auto entityPosWrtCamera = ( entityBox.pos - camBox.pos ) >> FIXEDPT_DEC_BITS;
  unsigned imgRowIdx = std::abs( std::min( 0, entityPosWrtCamera.y ) ) + ( hasSrcRect ? srcRect.y : 0 );
  const int clipLeft = std::abs( std::min( entityPosWrtCamera.x, 0 ) );
  for ( unsigned croppedBoxRowIdx{}; croppedBoxRowIdx < croppedBox.size.h; ++croppedBoxRowIdx, ++imgRowIdx ) {
    if ( imgRowIdx >= entityImg.lineLimits.size() ) {
      break;
    }
    const auto& lineLims = entityImg.lineLimits.at( imgRowIdx );
    int rowStart = hasSrcRect ? srcRect.x : 0;
    int rowLen = hasSrcRect ? std::min( srcRect.w, lineLims.len - srcRect.x ) : lineLims.len;
    int stringLength = std::min( static_cast<int>( croppedBox.size.w ), rowLen - clipLeft );
    if ( stringLength <= 0 ) {
      continue;
    }
    auto rowStr = std::string( entityImg.string, lineLims.start + rowStart + clipLeft, stringLength );
    text( win, croppedBox.pos.y - ( camBox.pos.y >> FIXEDPT_DEC_BITS ) + croppedBoxRowIdx,
          croppedBox.pos.x - ( camBox.pos.x >> FIXEDPT_DEC_BITS ), rowStr );
  }
}

auto renderer() -> Renderer& {
  static AsciiRenderer r;
  return r;
}

}  // namespace gfx
