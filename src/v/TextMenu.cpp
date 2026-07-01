#include "v/TextMenu.h"
#include "c/WindowManager.h"
#include <algorithm>
#include "bicycle.h"
#include <yaml-cpp/yaml.h>
#include "Constants.h"
#include <iostream>

TextMenu::TextMenu( const std::string& menuName, 
    const std::vector<Menu::MenuItem>& items,
    const int x,
    const int y,
    const int w,
    const int h) : Menu( menuName, items, x, y, w, h ) {}

void TextMenu::render() {
  const int LAST_DISP_IDX = std::min<int>( _firstDispIdx + getHeight() - WINDOW_PADDING, _items.size() );
  const auto CURSOR_WIDTH = _cursor.img.getSymbol().size();
  for ( int currRow = 1, dispIdx = _firstDispIdx; dispIdx < LAST_DISP_IDX; ++dispIdx ) {
    // CURSOR_WIDTH leaves room for cursor to the left of this menu item
    if ( dispIdx == _cursor.currItemIdx ) {
      setAttr( A_STANDOUT );
    }
    auto item = _items.at( dispIdx ).body;

    mvprint( currRow++, WINDOW_PADDING/2 + CURSOR_WIDTH, std::get<std::string>(_items.at( dispIdx ).body) );  
    unsetAttr( A_STANDOUT );
  }
  mvprint( _cursor.currItemIdx - _firstDispIdx + WINDOW_PADDING / 2, WINDOW_PADDING / 2, _cursor.img.getSymbol()  );
}

void TextMenu::onCursorMovement() {
}

// TODO I think we ought to split cursor logic from rendering logic
// TODO and i think cursor movement ought to be moved to Menu.cpp.
// TODO Handle the selection-filling/passing/processing here.
void TextMenu::onInput( const InputState& input ) {
  auto& wm = WindowManager::getInstance();
  const int NUM_ROWS_DISP = getHeight() - WINDOW_PADDING;
  if ( (MASK_J & input.currKeysPressed).any() ) {
    ++_cursor.currItemIdx;
    _cursor.currItemIdx = std::clamp<int>( _cursor.currItemIdx, 0, _items.size() - 1 );
    if ( ( _cursor.currItemIdx ) > ( _firstDispIdx + NUM_ROWS_DISP - 1 ) ) {
      ++_firstDispIdx;
    }
  }
  if ( (MASK_K & input.currKeysPressed).any() ) {
    --_cursor.currItemIdx;
    _cursor.currItemIdx = std::clamp<int>( _cursor.currItemIdx, 0, _items.size() - 1 );
    if ( _cursor.currItemIdx < _firstDispIdx ) {
      --_firstDispIdx;
    }
  }
  if ( (MASK_B & input.currKeysPressed).any() ) {
    wm.pop();
  }
  if ( (MASK_SPACE & input.currKeysPressed).any() ) {
    auto item = getItem();
    item.cb();
  }
}
