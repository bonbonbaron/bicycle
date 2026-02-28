#include "Dice.h"

Dice::Dice( const int min, const int max ) : _e1(_r()), _uniform_dist( min, max ) {}

auto Dice::roll() -> int {
  return _uniform_dist(_e1);
}
