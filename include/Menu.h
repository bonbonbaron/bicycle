#pragma once
#include <string>
#include <vector>
#include <curses.h>
#include <functional>
#include "Window.h"

struct MenuItem {
  std::string name;
  std::function<void()> func;
};

static const std::string CURSOR{ " > " };
static constexpr long unsigned int CURSOR_WIDTH{ 3 + ( WINDOW_PADDING / 2 )};

class Menu : public Window {
  public:
    Menu( const std::vector<MenuItem>& v, const int x, const int y, const int h );
    Menu() = delete;  // ensure they pass in items.
    void react( const int input ) override;
    const MenuItem& getCurrMenuItem() const;
    void update() override;
  private:
    WINDOW* _win;
    std::vector<MenuItem> _items{};
    int _currMenuItemIdx{};
    int _firstDispIdx{};
    int getFittedWidth( const std::vector<MenuItem>& v );
};
