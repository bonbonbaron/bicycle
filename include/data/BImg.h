#pragma once
// .bimg: bicycle's raw image, written by scripts/aseprite/sprite.lua (pixel export) and tools/pack.
//   "BIMG" u16 w u16 h u16 nColors u16 reserved, then
//   nColors > 0: palette (nColors x RGBA u32, R in the low byte) and w*h u8 palette indices (indexed image)
//   nColors = 0: w*h RGBA u32 pixels (true color, e.g. 3D textures)
// Index 0 / alpha 0 is transparent.
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct BImg {
  int w{};
  int h{};
  std::vector<uint32_t> palette;  // empty for true color
  std::vector<uint8_t> indices;   // w*h, indexed images
  std::vector<uint32_t> rgba;     // w*h, true color images

  auto indexed() const -> bool { return !palette.empty(); }
  auto pixel( int x, int y ) const -> uint32_t {  // RGBA, R in the low byte
    const size_t i = static_cast<size_t>( y ) * w + x;
    return indexed() ? palette[indices[i]] : rgba[i];
  }
  static auto parse( const std::string& bytes ) -> std::optional<BImg>;
  auto serialize() const -> std::string;
};
