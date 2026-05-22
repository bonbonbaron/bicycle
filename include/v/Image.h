#pragma once
#include "m/Position.h"
#include "v/ColorPalette.h"

using Symbol = std::string;
using Graybody = Symbol;  // Graybodies can combine with Color(Palettes) to create Images. Else, grayscale images.

// Image won't have position, because it needs to be inherited by Animation later.
class Image {
  public:
    Image( const Graybody& graybody, const ColorPalette& colorPalette );
    // Color
    auto getColor() const -> Color;
    void setColor( const std::string& color );
    void setColor( const Color& color );
    // Symbol
    auto getSymbol() const -> Symbol&;
    void setSymbol( const Symbol& symbol );
    // Miscellaneous
    auto isVisible() const -> bool;
    void setVisible( const bool visible );
  private:
    std::string _symbol;
    Color _color;
    bool _visible;
};

