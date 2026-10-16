#pragma once
#include <string>
#include "data/Mesh.h"
#include "data/Edgebreaker.h"

struct MeshEncodeInfo {
  bool raw{};          // Edgebreaker didn't apply; indices were stored instead
  size_t opsBytes{};   // compressed connectivity size (ops + offsets + raw indices)
  size_t posBytes{};
  size_t nrmBytes{};
  size_t uvBytes{};
  eb::Encoded enc;     // decoded-order mapping, for tests
};

// Reads a Wavefront OBJ (+ its .mtl). Texture paths are returned as written in the .mtl.
auto readObj( const std::string& objPath, MeshData& out, std::string& err ) -> bool;
auto meshEncode( const MeshData& m, int qbits, MeshEncodeInfo* info = nullptr ) -> std::string;
