#pragma once
#include "m/Position.h"
#include "v/ColorPalette.h"
#include "m/Size.h"
#include <vector>
#include "LineLimits.h"

// Image won't have position, because it needs to be inherited by Animation later.
struct Image {
  Image() = default;
  Image( const char* );
  Image( const char*, Color color );
  void strToImg( const char* str );
  const char* string;
  Size size{};
  Color color{Color::WHITE};
  bool visible{true};
  // TODO unsigned scale{};  // this'll be fixed point arithmetic
  std::vector<LineLimits> lineLimits{};
};

