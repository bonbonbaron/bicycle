#pragma once
#include <string>
#include "data/BImg.h"
// Strip-encodes an indexed image (see include/data/Strip.h) with unitsPerStrip units per strip.
auto stripEncode( const BImg& img, int unitsPerStrip ) -> std::string;
