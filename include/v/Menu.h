#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <variant>

#include "v/Image.h"
#include "m/Position.h"
#include "m/Entity.h"
#include "v/Window.h"

class Menu : public Window {
  public:
    using MenuItemBody = std::variant<Entity, std::string>;

    struct MenuItem {
      std::string text;
      int cbRef;
    };

    auto getItem() const -> MenuItem;
    void setOnCursorMovement( int cbRef );
    void addItem( const char* text, int cbRef );  // call after menu creation

    Menu( 
      const int x,
      const int y,
      const int w,
      const int h);

  protected:
    struct Cursor {
      Image img{};
      Position offset{};
      int currItemIdx{};
    };

    const MenuItem& getCurrMenuItem() const;
    void moveCursor( int amt );
    void cancel();
    std::vector< MenuItem > _items{};
    Cursor _cursor{ { " > ", Color::WHITE }, {0, 0}, 0 };
    std::string _id;
    int _onCursorMovementCbRef;
};
