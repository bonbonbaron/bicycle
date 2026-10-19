#pragma once
// .bimg writing, offline only (format in include/data/BImg.h; the engine only parses it, and
// scripts/aseprite/sprite.lua writes it from Aseprite).
#include <string>
#include "data/BImg.h"

namespace bimg {
auto serialize( const BImg& img ) -> std::string;
}  // namespace bimg
