#include "v/ColorPalette.h"
#include "v/Menu.h"
#include "c/WindowManager.h"

Menu::Menu( 
    const int x,
    const int y,
    const int w,
    const int h) :  Window( x, y, w, h ) {}

void Menu::addItem( const char* text, int cbRef ) {
  _items.push_back( { std::string(text), cbRef }  );
}

const Menu::MenuItem& Menu::getCurrMenuItem() const {
  return _items.at( _cursor.currItemIdx );
}

void Menu::setOnCursorMovement( int cbRef ) {
  _onCursorMovementCbRef = cbRef;
}

// Return a copy instead of reference in case the menu size changes/reallocates.
auto Menu::getItem() const -> MenuItem {
  auto item = _items.at( _cursor.currItemIdx );
  return item;
}
