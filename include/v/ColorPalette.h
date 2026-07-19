#pragma once
#include <cursesw.h>
#include <unordered_map>
#include <string>

enum Color {
  BLACK	  ,
  RED	    ,
  GREEN	  ,
  YELLOW  ,
  BLUE	  ,
  MAGENTA ,
  CYAN	  ,
  WHITE   ,
};

const std::unordered_map<std::string, Color> strToColor {
  {"BLACK",	 BLACK	  },
    {"RED",	   RED	    },
    {"GREEN",	 GREEN	  },
    {"YELLOW", YELLOW  },
    {"BLUE",	 BLUE	  },
    {"MAGENTA", MAGENTA },
    {"CYAN",	 CYAN	  },
    {"WHITE", WHITE   },
};

class ColorPalette {
  private:
    ColorPalette() = default;
    ColorPalette( const ColorPalette& ) = delete;
    auto operator=( const ColorPalette& ) -> ColorPalette& = delete;
  public:
    static auto getInstance() -> const ColorPalette& {
      const static ColorPalette cp;
      return cp;
    }
    static void init();
};


