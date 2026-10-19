#pragma once
// .bskin encoding, offline only (format in include/data/Skin.h; the engine only decodes).
#include <string>
#include "data/Skin.h"

namespace skin {
// quantized: translations to 16 bits per axis over the action's range, rotations smallest-three, both
// delta + zigzag varint coded per joint; withVertices: per-vertex joints/weights too (OBJ meshes).
auto encode( const Skin& s, bool quantized, bool withVertices ) -> std::string;
}  // namespace skin
