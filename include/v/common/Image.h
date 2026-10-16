#pragma once
#include "m/Position.h"
#include "v/common/ColorPalette.h"
#include "m/Size.h"
#include <vector>
#include "LineLimits.h"
#include <memory>
#include "data/BImg.h"

// Image won't have position, because it needs to be inherited by Animation later.
struct Image {
  Image() = default;
  Image( const char* );
  Image( const char*, Color color );
  void strToImg( const char* str );
  std::string string;
  Size size{};
  Color color{Color::WHITE};
  bool visible{true};
  // TODO unsigned scale{};  // this'll be fixed point arithmetic
  std::vector<LineLimits> lineLimits{};
  // Pixel modes: the image's pixels (null for text images, which pixel modes draw as glyphs). One pixel covers
  // 2x1 cells, so size is (2 * pixels->w, pixels->h) cells and the ASCII and pixel versions line up.
  std::shared_ptr<const BImg> pixels{};
};

