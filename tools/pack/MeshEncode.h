#pragma once
#include <string>
#include "data/Mesh.h"
#include "data/Obj.h"
#include "data/Edgebreaker.h"

struct MeshEncodeInfo {
  bool raw{};          // Edgebreaker didn't apply; indices were stored instead
  size_t opsBytes{};   // compressed connectivity size (ops + offsets + raw indices)
  size_t posBytes{};
  size_t nrmBytes{};
  size_t uvBytes{};
  size_t skinBytes{};
  eb::Encoded enc;     // decoded-order mapping, for tests
};

auto meshEncode( const MeshData& m, int qbits, MeshEncodeInfo* info = nullptr ) -> std::string;
