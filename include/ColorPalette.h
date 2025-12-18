#pragma once
#include <ncurses.h>

enum Color : short {
  BLACK	  = COLOR_BLACK,
  RED	    = COLOR_RED,
  GREEN	  = COLOR_GREEN,
  YELLOW  = COLOR_YELLOW,
  BLUE	  = COLOR_BLUE,
  MAGENTA = COLOR_MAGENTA,
  CYAN	  = COLOR_CYAN,
  WHITE   = COLOR_WHITE,
};

enum ColorPair : short {
  WINDOW_ = 0,
  DIALOGUE =1,
  MENU_SEL = 3,
  MENU_UNSEL = 4,
  // TODO more to come later
};


class ColorPalette {
  private:
    ColorPalette();
    ColorPalette( const ColorPalette& ) = delete;
    auto operator=( const ColorPalette& ) -> ColorPalette& = delete;
  public:
    static auto getInstance() -> const ColorPalette& {
      const static ColorPalette cp;
      return cp;
    }
    [[nodiscard]] auto getColorPair( const ColorPair ) const -> int;
    void set( const ColorPair cp ) const;
};

