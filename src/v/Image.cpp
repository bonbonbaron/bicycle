#include "v/Image.h"

Image::Image( const Graybody& graybody, const Color& color ) :
  _symbol( graybody ), _color( color ) {}

// Color
auto Image::getColor() const -> Color {
  return _color;
}

void Image::setColor( const Color& color ) {
  _color = color;
}

// Symbol
auto Image::getSymbol() const -> Symbol {
  return _symbol;
}

void Image::setSymbol( const Symbol& symbol ) {
  _symbol = symbol;
}

// Miscellaneous
auto Image::isVisible() const -> bool {
  return _visible;
}

void Image::setVisible( const bool visible ) {
  _visible = visible;
}

