#pragma once
// Edgebreaker encoding, offline only (see include/data/Edgebreaker.h for the scheme; the engine only decodes).
#include "data/Edgebreaker.h"

namespace eb {

// Encoder output: the connectivity plus how decoded ids/triangles map back to the input.
struct Encoded {
  Connectivity conn;
  std::vector<int32_t> vertexOrigin;   // decoded vertex -> input vertex (-1 for dummies)
  std::vector<int32_t> triOrigin;      // decoded triangle -> input triangle (-1 for dummy triangles)
  std::vector<uint8_t> triRotation;    // decoded corner j is input corner (j + rotation) % 3
};

auto encode( const std::vector<uint32_t>& tris, uint32_t nVerts ) -> Encoded;

}  // namespace eb
