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

void Menu::react( const int input ) {
  auto& wm = WindowManager::getInstance();
  const int NUM_ROWS_DISP = getHeight() - WINDOW_PADDING;
  switch( input ) {
    case 'J':  // move cursor down (so caps lock doesn't piss people off)
    case 'j':  // move cursor down
      _currMenuItemIdx = std::clamp<int>( ++_currMenuItemIdx, 0, _items.size() - 1 );
      if ( ( _currMenuItemIdx ) > ( _firstDispIdx + NUM_ROWS_DISP - 1 ) ) {
        ++_firstDispIdx;
      }
      break;
    case 'K':  // move cursor up (so caps lock doesn't piss people off)
    case 'k':  // move cursor up
      _currMenuItemIdx = std::clamp<int>( --_currMenuItemIdx, 0, _items.size() - 1 );
      if ( _currMenuItemIdx < _firstDispIdx ) {
        --_firstDispIdx;
      }
      break;
    case 'B':  // cancel
    case 'b':  // cancel
      wm.pop();
      break;
    case ' ':  // space bar
      _items.at( _currMenuItemIdx ).func();
      break;
    default:
      Window::react( input );
  }
}

void Menu::update() {
  const int LAST_DISP_IDX = std::min<int>( _firstDispIdx + getHeight() - WINDOW_PADDING, _items.size() );
  for ( int currRow = 1, dispIdx = _firstDispIdx; dispIdx < LAST_DISP_IDX; ++dispIdx ) {
    // CURSOR_WIDTH leaves room for cursor to the left of this menu item
    mvprint( currRow++, CURSOR_WIDTH, _items.at( dispIdx ).name );  
  }
  mvprint( _currMenuItemIdx - _firstDispIdx + WINDOW_PADDING / 2, WINDOW_PADDING / 2, CURSOR  );
}

