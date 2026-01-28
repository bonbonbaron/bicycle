#include <exception>
#include "Body.h"

// Setters

void Body::setPosition( const std::shared_ptr<Position>& pos ) {
  _pos = pos;
}

void Body::setColor( const std::string& s ) {
  try {
    _color = strToColor.at( s );
      
  }
  catch ( const std::out_of_range& e ) {
    // Throw a bad argument error instead.
    throw std::invalid_argument( "Color " + s + " is not in ColorPalette.h's strToColor map." );
  }
}

auto Body::getColor() const -> Color {
  return _color;
}


// trivial (getters)
auto Body::getSymbol() const -> const Image& {
  return _sym;
}

void Body::setSymbol( const Image& sym ) {
  _sym = sym;
}

void Body::setSymbol( const Image&& sym ) {
  _sym = sym;
}

auto Body::getPosition() const -> const std::shared_ptr<Position>& {
  return _pos;
}
