#pragma once
// .bimg: bicycle's raw image, written by scripts/aseprite/sprite.lua (pixel export) and tools/pack.
//   "BIMG" u16 w u16 h u16 nColors u16 reserved, then
//   nColors > 0: palette (nColors x RGBA u32, R in the low byte) and w*h u8 palette indices (indexed image)
//   nColors = 0: w*h RGBA u32 pixels (true color, e.g. 3D textures)
// Index 0 / alpha 0 is transparent.
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "data/Mem.h"

struct BImg {
  int w{};
  int h{};
  // Pixels live in the GRAPHIC pool.
  mem::Vector<uint32_t, mem::GRAPHIC> palette;  // empty for true color
  mem::Vector<uint8_t, mem::GRAPHIC> indices;   // w*h, indexed images
  mem::Vector<uint32_t, mem::GRAPHIC> rgba;     // w*h, true color images
  // A palette variant (genome palette = "..."): its own palette over another image's indices, which it shares
  // instead of copying (the graybody, loaded once; see newPixelImagePal in src/bicycle.cpp).
  std::shared_ptr<const BImg> gray;

  auto idx() const -> const mem::Vector<uint8_t, mem::GRAPHIC>& { return gray ? gray->indices : indices; }

  auto indexed() const -> bool { return !palette.empty(); }
  auto pixel( int x, int y ) const -> uint32_t {  // RGBA, R in the low byte
    const size_t i = static_cast<size_t>( y ) * w + x;
    if ( !indexed() ) return rgba[i];
    const uint8_t k = idx()[i];
    return k < palette.size() ? palette[k] : 0u;
  }
  static auto parse( std::string_view bytes ) -> std::optional<BImg>;
  auto serialize() const -> std::string;
};
