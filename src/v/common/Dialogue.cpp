#include "v/common/Dialogue.h"
#include <cassert>
#include <iostream>
#include <algorithm>

// This constructor's meant for short strings since it prints it all on one row.
Dialogue::Dialogue( const std::string&& s ) : Window( gfx::cols() / 2 - s.length() / 2, gfx::lines() / 2, s.length() + WINDOW_PADDING, WINDOW_PADDING + 1 ), _content( s ) {
  init();
}

Dialogue::Dialogue( const std::string& s ) : Window( gfx::cols() / 2 - s.length() / 2, gfx::lines() / 2, s.length() + WINDOW_PADDING, WINDOW_PADDING + 1 ), _content( s ) {
  init();
}

Dialogue::Dialogue( const std::string& s, const int x, const int y ) : Window(x, y, s.length() + 2, 3), _content( s ){
  init();
}

Dialogue::Dialogue( const std::string& s, const int x, const int y, const int w, const int h ) : Window(x, y, w, h), _content( s ){
  init();
}

Dialogue::Dialogue( const std::string&& s, const int x, const int y, const int w, const int h ) : Window(x, y, w, h), _content( s ){
  init();
}

void Dialogue::onTermResize() {
  const int H = _wantH;
  if ( _bottomAnchored ) {
    const int h = std::min( H, std::max( 3, gfx::lines() - 1 ) );
    place( 1, std::max( 0, gfx::lines() - h - 1 ), std::max( WINDOW_PADDING + 1, gfx::cols() - 4 ), h );
  }
  else {
    Window::onTermResize();
  }
  lineLimits.clear();  // keeps its capacity
  delimitLines();
  initLineNum = std::min( initLineNum, lastPageStart() );
}

void Dialogue::init() {
  // Dialogue.new (scripts/Dialogue.lua) spans the bottom: x 1, width cols - 2 (the window keeps w - 2), y + h one
  // row above the bottom.
  _wantH = static_cast<int>( getHeight() );
  _bottomAnchored = requestedX() == 1 && static_cast<int>( getWidth() ) == gfx::cols() - 4 &&
                    requestedY() + static_cast<int>( getHeight() ) == gfx::lines() - 1;
  // const auto& cp = ColorPalette::getInstance();  // TODO 
  delimitLines();
}

// TODO provide dev-friendly key masks
void Dialogue::onInput( Input& input ) {
  switch( input.lastPressed ) {
    case LogicalKey::J:
      ++initLineNum;
      initLineNum = std::clamp<unsigned>( initLineNum, 0, lastPageStart() );
      break;
    case LogicalKey::K:
      if ( initLineNum > 0 ) {
        --initLineNum;
      }
      break;
    case LogicalKey::F:
      initLineNum = std::min( initLineNum + getTextRows(), lastPageStart() );
      break;
    case LogicalKey::B:
      initLineNum -= std::min( initLineNum, getTextRows() );
      break;
    case LogicalKey::E:
    case LogicalKey::Space:  // next page, or done after the last one
      if ( isOnLastPage() ) {
        _done = true;
      }
      else {
        initLineNum += getTextRows();
      }
      break;
    default:
      Window::onInput( input );
  }
}

// Rows of text that fit inside the border, minus one for the "more"/"close" hint.
auto Dialogue::getTextRows() const -> unsigned {
  return std::max( 1u, getHeight() > 3 ? getHeight() - 3 : 1u );
}

auto Dialogue::lastPageStart() const -> unsigned {
  return lineLimits.size() > getTextRows() ? static_cast<unsigned>( lineLimits.size() ) - getTextRows() : 0;
}

auto Dialogue::isOnLastPage() const -> bool {
  return initLineNum + getTextRows() >= lineLimits.size();
}

auto Dialogue::isDone() const -> bool {
  return _done;
}

// Splits _content into rows that fit inside the border: at spaces, at newlines (a blank row for each extra one),
// and inside words too long for a row.
void Dialogue::delimitLines() {
  assert( getWidth() > WINDOW_PADDING );
  const int WIDTH = static_cast<int>( getWidth() - WINDOW_PADDING );
  unsigned start{};
  while ( start < _content.length() ) {
    const auto newline = _content.find( '\n', start );
    const unsigned paraEnd = newline == std::string::npos ? _content.length() : newline;
    if ( static_cast<int>( paraEnd - start ) <= WIDTH ) {  // the rest of the paragraph fits
      lineLimits.push_back( { start, static_cast<int>( paraEnd - start ) } );
      start = paraEnd + 1;
      continue;
    }
    auto space = _content.rfind( ' ', start + WIDTH );
    if ( space == std::string::npos || space <= start ) {  // one long word: break it
      lineLimits.push_back( { start, WIDTH } );
      start += WIDTH;
      continue;
    }
    lineLimits.push_back( { start, static_cast<int>( space - start ) } );
    start = space + 1;
  }
}

void Dialogue::render() {
  int windowRow{1};
  for ( unsigned i = initLineNum; i < initLineNum + getTextRows() && i < lineLimits.size(); ++i ) {
    mvprint( windowRow++, 1, std::string( _content, lineLimits.at(i).start, lineLimits.at(i).len ) );
  }
  const std::string hint = isOnLastPage() ? "[E] close" : "[E] more";
  mvprint( getHeight() - 2, std::max( 1, static_cast<int>( getWidth() ) - 1 - static_cast<int>( hint.length() ) ), hint );
}
