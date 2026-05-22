#pragma once

#include <vector>
#include <functional>

#include "m/Position.h"

struct Cursor {
  Image img;
  Position offset;
  int currItemIdx{};
};

class Menu {
  public:
    Menu( const std::string& menuName );
    Menu() = delete;  // ensure they pass in items.
    const MenuItem& getCurrMenuItem() const;
    void onInput( );
    void moveCursor( int amt );

  private:
    Cursor _cursor;
    std::vector< Entity > _items{};
    int _firstDispIdx{};
    void addItem( const MenuItem& entity );
};
