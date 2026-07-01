#include "v/ColorPalette.h"
#include "v/Menu.h"
#include "c/WindowManager.h"

Menu::Menu( const std::string& menuName, 
    const std::vector<MenuItem>& items,
    const int x,
    const int y,
    const int w,
    const int h) :  Window( x, y, w, h ), _items(items), _id( menuName ) {
  auto& wm = WindowManager::getInstance();
  _parent = dynamic_pointer_cast<Menu>( wm.back() );
}

void Menu::addItem( const Menu::MenuItem& item ) {
  _items.push_back( item );
}

const Menu::MenuItem& Menu::getCurrMenuItem() const {
  return _items.at( _cursor.currItemIdx );
}

void Menu::onCursorChange() {
  // do nothing
}

// Return a copy instead of reference in case the menu size changes/reallocates.
auto Menu::getItem() const -> MenuItem {
  auto item = _items.at( _cursor.currItemIdx );
  return item;
}
