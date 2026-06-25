#include "v/Dialogue.h"
#include <cassert>
#include <iostream>
#include <algorithm>
#include "c/WindowManager.h"

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
  // const auto& cp = ColorPalette::getInstance();  // TODO 
  delimitLines();
}

// TODO provide dev-friendly key masks
void Dialogue::react( const InputState& input ) {
  auto& wm = WindowManager::getInstance();
  if ( (MASK_J & input.currKeysPressed).any() ) {
    ++initLineNum;
    initLineNum = std::clamp<unsigned>( initLineNum, 0, lineLimits.size()  - 4 );
  }
  else if ( (MASK_K & input.currKeysPressed).any() ) {
    if ( initLineNum > 0 ) {
      --initLineNum;
    }
    initLineNum = std::clamp<unsigned>( initLineNum, 0, lineLimits.size() );
  }
  else if ( (MASK_F & input.currKeysPressed).any() ) {
    initLineNum = std::clamp<unsigned>( initLineNum + getHeight(), 0, lineLimits.size() - 1 );
  }
  else if ( (MASK_B & input.currKeysPressed).any() ) {
    initLineNum -= std::min( initLineNum, getHeight() );
    initLineNum = std::clamp<unsigned>( initLineNum, 0, lineLimits.size() - 1 );
  }
  else if ( (MASK_SPACE & input.currKeysPressed).any() ) {
    wm.pop();
  }
  else {
    Window::react( input );
  }
  render();
}

void Dialogue::delimitLines() {
  constexpr int NUM_INSTANCES_TO_FIND{1};
  assert( getWidth() > WINDOW_PADDING );
  const unsigned WIDTH = getWidth() - WINDOW_PADDING;
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

void Dialogue::render() {
  int windowRow{1};
  for ( unsigned i = initLineNum; i < getHeight() && i < lineLimits.size(); ++i ) {
    mvprint( windowRow++, 1, std::string( _content, lineLimits.at(i).start, lineLimits.at(i).len ) );
  }
}
