#include "ColorPalette.h"

ColorPalette::ColorPalette() {
  init_pair( WINDOW_,   WHITE, BLACK );
  init_pair( DIALOGUE, GREEN, BLACK );
  init_pair( MENU_SEL, BLACK, BLUE );
  init_pair( MENU_UNSEL, GREEN, BLACK );
}

auto ColorPalette::getColorPair( const ColorPair cp ) const -> int {
  return COLOR_PAIR( cp );  // ensure we've init'd every enum's pair.
}

void ColorPalette::set( const ColorPair cp ) const {
  color_set( cp, nullptr );
}
