#include "v/TextMenu.h"
#include <algorithm>
#include "bicycle.h"
#include "Constants.h"
#include <iostream>

TextMenu::TextMenu(
    const int x,
    const int y,
    const int w,
    const int h) : Menu( x, y, w, h ) {}

void TextMenu::render() {
  const int LAST_DISP_IDX = std::min<int>( _firstDispIdx + getHeight() - WINDOW_PADDING, _items.size() );
  const auto CURSOR_WIDTH = _cursor.img.getSymbol().size();
  for ( int currRow = 1, dispIdx = _firstDispIdx; dispIdx < LAST_DISP_IDX; ++dispIdx ) {
    // CURSOR_WIDTH leaves room for cursor to the left of this menu item
    if ( dispIdx == _cursor.currItemIdx ) {
      setAttr( A_STANDOUT );
    }

    mvprint( currRow++, WINDOW_PADDING/2 + CURSOR_WIDTH, _items.at( dispIdx ) );  
    unsetAttr( A_STANDOUT );
  }
  mvprint( _cursor.currItemIdx - _firstDispIdx + WINDOW_PADDING / 2, WINDOW_PADDING / 2, _cursor.img.getSymbol()  );
}

// TODO I think we ought to split cursor logic from rendering logic
// TODO and i think cursor movement ought to be moved to Menu.cpp.
// TODO Handle the selection-filling/passing/processing here.
void TextMenu::onInput( Input& input ) {
  const int NUM_ROWS_DISP = getHeight() - WINDOW_PADDING;
  switch( input.lastPressed ) {
    case LogicalKey::J:
      ++_cursor.currItemIdx;
      _cursor.currItemIdx = std::clamp<int>( _cursor.currItemIdx, 0, _items.size() - 1 );
      if ( ( _cursor.currItemIdx ) > ( _firstDispIdx + NUM_ROWS_DISP - 1 ) ) {
        ++_firstDispIdx;
      }
      break;
    case LogicalKey::K:
      --_cursor.currItemIdx;
      _cursor.currItemIdx = std::clamp<int>( _cursor.currItemIdx, 0, _items.size() - 1 );
      if ( _cursor.currItemIdx < _firstDispIdx ) {
        --_firstDispIdx;
      }
      break;
  }
  input.focus = getId();
}
