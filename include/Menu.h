#pragma once

#include <string>
#include <vector>
#include <curses.h>
#include <functional>

#include "Window.h"
#include "Entity.h"

static const std::string CURSOR{ " > " };
static constexpr long unsigned int CURSOR_WIDTH{ 3 + ( WINDOW_PADDING / 2 )};

using MenuItem = std::shared_ptr<Entity>;

class Menu : public Window {
  public:
    Menu( const std::string& menuName );
    Menu() = delete;  // ensure they pass in items.
    const MenuItem& getCurrMenuItem() const;
    void update() override;
    void react( const int input ) override;
  private:
    WINDOW* _win;
    std::vector< MenuItem > _items{};
    int _currMenuItemIdx{};
    int _firstDispIdx{};
    void addItem( const MenuItem& entity );
};
