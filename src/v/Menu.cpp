#include "v/ColorPalette.h"
#include "v/Menu.h"


void Menu::addItem( const Menu::MenuItem& item ) {
  _items.push_back( item );
}

const Menu::MenuItem& Menu::getCurrMenuItem() const {
  return _items.at( _currMenuItemIdx );
}

constexpr std::string_view ON_SELECT{ "onSelect" };

void onCursorChange() {
  // do nothing
}
