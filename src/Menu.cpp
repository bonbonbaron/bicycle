#include "Menu.h"
#include "ColorPalette.h"
#include <cassert>
#include <algorithm>
#include "WindowManager.h"

Menu::Menu( const std::vector<MenuItem>& v, const int x, const int y, const int h ) : Window(x, y, getFittedWidth( v ), h), _items( v ) {
}

const MenuItem& Menu::getCurrMenuItem() const {
  return _items.at( _currMenuItemIdx );
}

int Menu::getFittedWidth( const std::vector<MenuItem>& v ) {
  int maxWidth{};
  for ( const auto& item : v ) {
    maxWidth = std::max<int>( item.name.length() + CURSOR_WIDTH + WINDOW_PADDING, maxWidth );
  }
  return maxWidth;
}

// TODO make a selector function for going up and down through menu
//      to take advantage of this old code. Something about cursors.
//      How do we use cursors again?
/*
    const int NUM_ROWS_DISP = getHeight() - WINDOW_PADDING;

    // Moving cursor DOWN
    _currMenuItemIdx = std::clamp<int>( ++_currMenuItemIdx, 0, _items.size() - 1 );
    if ( ( _currMenuItemIdx ) > ( _firstDispIdx + NUM_ROWS_DISP - 1 ) ) {
      ++_firstDispIdx;
    }

    // Moving cursor UP
    _currMenuItemIdx = std::clamp<int>( --_currMenuItemIdx, 0, _items.size() - 1 );
    if ( _currMenuItemIdx < _firstDispIdx ) {
      --_firstDispIdx;
    }

*/
void Menu::update() {
  const int LAST_DISP_IDX = std::min<int>( _firstDispIdx + getHeight() - WINDOW_PADDING, _items.size() );
  for ( int currRow = 1, dispIdx = _firstDispIdx; dispIdx < LAST_DISP_IDX; ++dispIdx ) {
    // CURSOR_WIDTH leaves room for cursor to the left of this menu item
    print( _items.at( dispIdx ).name, CURSOR_WIDTH, currRow++ );  
  }
  print( CURSOR, WINDOW_PADDING / 2, _currMenuItemIdx - _firstDispIdx + WINDOW_PADDING / 2 );
}

