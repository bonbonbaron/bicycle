#pragma once
#include <string>
#include "ColorPalette.h"

static constexpr int WINDOW_PADDING{2};

class Window : Body {
  public:
    // i know this doesn't align with ncurses' paramter order, but I like my order better :)
    Window( const int w, const int h );
    Window( const int x, const int y, const int w, const int h );
    Window() = delete;  // ensure they pass in items.
    ~Window();
    void show();
    void hide();
    void clear();
    virtual void update() = 0;
    void repaint();
    const char* getCurrItemName() const;
    const char* getCurrItemDesc() const;
    auto getX() const -> int;
    auto getY() const -> int;
    auto getWidth() const -> int;
    auto getHeight() const -> int;
    void setColorPair( const ColorPair );
    auto getColorPair() const -> chtype;
    void setBorder( const bool showBorder );
    auto isBorderShowing() const -> bool;
    void print( const std::string&, const int x, const int y ) const;
  private:
    WINDOW* _win;
    chtype _colorPair;
    bool _showBorder{};
    std::string name;
    int _w;
    int _h;
};
