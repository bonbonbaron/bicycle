#pragma once
// Strip compression for indexed images, ported from jollybean (src/data/strip.c, utils/src/sd/sd.c):
//   1. Cut the palette indices into strips of N units (pixels), row-major across the whole image.
//   2. Deduplicate them: the stripset holds each distinct strip once, the stripmap (u16) says which strip goes
//      where. Sprite sheets and tilesets repeat a lot (transparent runs, repeated tiles).
//   3. Bit-pack the stripset at the fewest bits per unit (1, 2, 4 or 8) the palette needs.
//   4. Deflate the packed stripset and the stripmap (zlib; inflated with tinfl).
// jollybean staggers units across machine words for SIMD unpacking; this port packs bytes LSB-first so it's
// the same on every platform.
//
// Encoded layout: "STRP" u16 w u16 h u16 nColors u8 unitsPerStrip u8 bpu u32 nStrips u32 nIndices
//                 palette (nColors x u32) u32 ssRaw u32 ssComp u32 smRaw u32 smComp, stripset, stripmap
#include <optional>
#include <string>
#include "data/BImg.h"

namespace strip {
// Decodes to a .bimg byte string.
auto decode( const std::string& encoded ) -> std::optional<std::string>;
}  // namespace strip
