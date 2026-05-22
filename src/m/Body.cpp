#include <exception>
#include "m/Body.h"

// TODO this should boil down to a header file with a struct: image, animation, position, and collision
//      There should basically be no more body functions.

// Setters

void Body::setPosition( const std::shared_ptr<Position>& pos ) {
  _pos = pos;
}

auto Body::getPosition() const -> const std::shared_ptr<Position>& {
  return _pos;
}
