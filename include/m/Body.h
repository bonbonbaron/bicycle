#pragma once
#include <string>
#include "v/ColorPalette.h"
#include "Config.h"
#include "v/Image.h"

/* TODO
 struct CollRect : public Rect {
   // Position is relative to Body dstRect
   unsigned type;
 }
*/

// Body is the visual representation of an entity.
struct Body {
    Image img;  
    Rect  dstRect;
    // TODO CollRect collRect;
};

