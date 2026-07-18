#include "v/ColorPalette.h"
#include "v/Menu.h"
#include "c/WindowManager.h"

Menu::Menu( 
    const int x,
    const int y,
    const int w,
    const int h) :  Window( x, y, w, h ) {}

void Menu::addItem( const char* text ) {
  _items.push_back( std::string(text)  );
}

const unsigned Menu::getSelection() const {
  return _cursor.currItemIdx;
}
