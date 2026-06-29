#include "v/ColorPalette.h"
#include "v/Menu.h"
#include "c/WindowManager.h"

Menu::Menu( const std::string& menuName, 
    const std::vector<MenuItem>& items,
    const int x,
    const int y,
    const int w,
    const int h,
    // TODO I really hate this param. Let's get rid of it somehow.
    const bool hasChildEntities ) :  Window( x, y, w, h, hasChildEntities ), _items(items), _id( menuName ) {
  auto& wm = WindowManager::getInstance();
  _parent = dynamic_pointer_cast<Menu>( wm.back() );
}

void Menu::addItem( const Menu::MenuItem& item ) {
  _items.push_back( item );
}

const Menu::MenuItem& Menu::getCurrMenuItem() const {
  return _items.at( _cursor.currItemIdx );
}

void Menu::setChildSelection( const Selection& selection ) {
  _childSelection = selection;
}

void Menu::onCursorChange() {
  // do nothing
}
