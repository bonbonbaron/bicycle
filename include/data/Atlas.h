#pragma once
// Texture atlas, ported from jollybean (src/x/xRender.c: atlasNew, atlasPlanPlacements, _atlasLinkNodes,
// assembleTextureAtlas, updateCmSrcRectIndices; src/data/bt.c binary-tree headers).
//
// Rectangles are sorted by their larger dimension (biggest first), then placed into a binary tree whose nodes
// each hold one image: a node's RIGHT_RECT child sits to its right on the same shelf, its LOWER_RECT child below
// it. A search for room walks right/down from the right extremity, backs out up/left to the first unexplored
// downward branch, and when nothing fits grows the atlas right or down (whichever keeps it squarer and fits).
// The root's remW/remH end up as the atlas's size. Planning works in the TEMPORARY pool, as in jollybean; the
// assembled pixels go to GRAPHIC.
#include <cstdint>
#include <vector>
#include "data/Mem.h"

namespace atlas {

using Key = uint32_t;  // jollybean's Key is a U8 (255 images); bicycle allows more
enum Child : uint8_t { RIGHT_RECT = 0, LOWER_RECT = 1 };

// bt.h: a header on each element of an otherwise ordinary array. jollybean marks orphans by filling the header
// with 0x33 bytes; here it's an explicit flag.
struct BtElHeader {
  Key childA[2]{};
  Key parent{};
  bool orphan{true};
};

struct Rect {
  int32_t x{}, y{}, w{}, h{};
};

struct AtlasElem {
  BtElHeader header;
  Key srcIdx{};      // index of the image in the caller's (unsorted) array
  Rect rect;
  uint32_t maxDim{}, // larger of width and height
    remW{},          // remaining atlas width as if this rect weren't here
    remH{};          // remaining atlas height as if this rect weren't here
};

struct Atlas {
  Key extremityA[2]{};
  mem::Vector<AtlasElem, mem::TEMPORARY> btP;
  auto width() const -> uint32_t { return btP.empty() ? 0 : btP[0].remW; }
  auto height() const -> uint32_t { return btP.empty() ? 0 : btP[0].remH; }
};

// Sizes of the images to pack, in the caller's order. pad adds that many pixels right and below each image (so
// GPU sampling never bleeds into a neighbor).
struct Size { uint32_t w, h; };
auto atlasNew( const std::vector<Size>& sizes, uint32_t pad = 0 ) -> Atlas;
void atlasPlanPlacements( Atlas& atlas );
// Where each source image landed (indexed by srcIdx), padding excluded.
auto atlasPlacements( const Atlas& atlas, uint32_t pad = 0 ) -> std::vector<Rect>;

}  // namespace atlas
