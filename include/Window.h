#pragma once
#include <string>
#include "ColorPalette.h"

static constexpr int WINDOW_PADDING{2};

class Window {
  public:
    // i know this doesn't align with ncurses' paramter order, but I like my order better :)
    Window() = default;
    Window( const int w, const int h );
    Window( const int x, const int y, const int w, const int h );
    ~Window();
    void create();
    void show();
    void hide();
    void clear();
    void move( int y, int x );
    virtual void update() = 0;
    virtual void react( const int input ) = 0;
    void repaint();
    const char* getCurrItemName() const;
    const char* getCurrItemDesc() const;

    auto getX() const -> int;
    auto getY() const -> int;
    auto getWidth() const -> int;
    auto getHeight() const -> int;

    void setX( const int x );
    void setY( const int y );
    void setWidth( const int w );
    void setHeight( const int h );

    void setColorPair( const ColorPair );
    auto getColorPair() const -> chtype;

    void setBorder( const bool showBorder );
    auto isBorderShowing() const -> bool;
    void mvprint( const int y, const int x, const std::string& s ) const;
    void addmvch( const int y, const int x, char c ) const;  // mvaddch() might be a macro, blows compilation
    void putc( const char c ) const;
    void print( const std::string& s ) const;
  private:
    WINDOW* _win;
    chtype _colorPair;
    bool _showBorder{};
    std::string name;
    int _w{};
    int _h{};
    int _x{};
    int _y{};
};
