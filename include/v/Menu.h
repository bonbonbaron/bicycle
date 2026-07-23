#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <variant>

#include "v/Image.h"
#include "m/Position.h"
#include "m/Entity.h"
#include "v/Window.h"

static std::string CURSOR{ " > " };

class Menu : public Window {
  public:
    using MenuItemBody = std::variant<Entity, std::string>;

    void setOnCursorMovement( int cbRef );
    void addItem( const char* text );  // call after menu creation
    const unsigned getSelection() const;

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

    void moveCursor( int amt );
    void cancel();
    std::vector<std::string> _items{};

    Cursor _cursor{ { CURSOR.data(), Color::WHITE }, {0, 0, 0}, 0 };
    std::string _id;
};
