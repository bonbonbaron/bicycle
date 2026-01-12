#pragma once
#include <string>
#include "ColorPalette.h"
struct Position {
  int x{};
  int y{};
};

// Body is the visual representation of an entity.
class Body {
  public:
    auto getPosition() const -> Position&;
    void setPosition( const int x, const int y );
    auto getColor() const -> Color;
    void setColor( const Color color );
    auto getSymbol() const -> const std::string&;
    void setSymbol( const std::string& str );
    void setSymbol( const std::string&& str );
  private:
    std::string symbol;  
    Position _pos{};
    Color _color{};
};


//TODO yaml
