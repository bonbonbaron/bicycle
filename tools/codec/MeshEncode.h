#pragma once
#include <string>
#include "data/Mesh.h"
#include "data/Obj.h"
#include "codec/EdgebreakerEncode.h"

struct MeshEncodeInfo {
  bool raw{};          // Edgebreaker didn't apply; indices were stored instead
  size_t opsBytes{};   // compressed connectivity size (ops + offsets + raw indices)
  size_t posBytes{};
  size_t nrmBytes{};
  size_t uvBytes{};
  size_t skinBytes{};
  eb::Encoded enc;     // decoded-order mapping, for tests
};

namespace mesh {
// Octahedral normal (NORMAL_BITS per component); mesh::octDecode is its runtime inverse.
auto octEncode( const float n[3] ) -> std::array<int32_t, 2>;
}  // namespace mesh

auto meshEncode( const MeshData& m, int qbits, MeshEncodeInfo* info = nullptr ) -> std::string;
