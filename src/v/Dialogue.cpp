#include "Dialogue.h"
#include <cassert>
#include <iostream>
#include <algorithm>
#include "WindowManager.h"

// This constructor's meant for short strings since it prints it all on one row.
Dialogue::Dialogue( const std::string&& s ) : Window( COLS / 2 - s.length() / 2, LINES / 2, s.length() + WINDOW_PADDING, WINDOW_PADDING + 1 ), _content( s ) {
  init();
}

Dialogue::Dialogue( const std::string& s ) : Window( COLS / 2 - s.length() / 2, LINES / 2, s.length() + WINDOW_PADDING, WINDOW_PADDING + 1 ), _content( s ) {
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

void Dialogue::init() {
  const auto& cp = ColorPalette::getInstance();
  delimitLines();
}

void Dialogue::react( const int input ) {
  auto& wm = WindowManager::getInstance();
  switch( input ) {
    case 'j':
      initLineNum = std::clamp<int>( ++initLineNum, 0, lineLimits.size()  - 4 );
      break;
    case 'k':
      initLineNum = std::clamp<int>( --initLineNum, 0, lineLimits.size() );
      break;
    case 'f':
      initLineNum = std::clamp<int>( initLineNum + getHeight() - WINDOW_PADDING, 0, lineLimits.size() - 1 );
      break;
    case 'b':
      initLineNum = std::clamp<int>( initLineNum - getHeight() + WINDOW_PADDING, 0, lineLimits.size() - 1 );
      break;
    case ' ':
      wm.pop();
      return;
      break;
    default:
      Window::react( input );
  }
  update();
}

void Dialogue::delimitLines() {
  constexpr int NUM_INSTANCES_TO_FIND{1};
  const int WIDTH = getWidth() - WINDOW_PADDING;
  assert( WIDTH > 0 );
  LineLimits currLineLims;
  for ( ;; ) {
    if ( ( _content.length() - currLineLims.start ) <= WIDTH ) {
      lineLimits.push_back( { currLineLims.start, static_cast<int>(_content.length()) - currLineLims.start } );
      break;
    }
    currLineLims.len = _content.rfind( " ", currLineLims.start + WIDTH, NUM_INSTANCES_TO_FIND ) - currLineLims.start;
    lineLimits.push_back( currLineLims );
    assert( ( currLineLims.len ) <= WIDTH );
    assert ( currLineLims.len > 0 );
    currLineLims.start += currLineLims.len + 1;
  }
}

void Dialogue::update() {
  int windowRow{1};
  for ( int i = initLineNum; i < getHeight() && i < lineLimits.size(); ++i ) {
    mvprint( windowRow++, 1, std::string( _content, lineLimits.at(i).start, lineLimits.at(i).len ) );
  }
}
