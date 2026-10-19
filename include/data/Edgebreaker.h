#pragma once
// Edgebreaker connectivity coding for triangle meshes (Rossignac 1999; see jollybean/school/eb/eb.c, whose
// half-edge/CLERS study this follows). The encoder walks the mesh one triangle at a time across a gate on the
// active boundary and writes one symbol per triangle:
//   C  the tip vertex is new                           L  the tip is the vertex before the gate
//   R  the tip is the vertex after the gate's end      E  the triangle closes its loop
//   S  the tip is elsewhere on the loop: the loop splits in two (we store how far along the loop, like the
//      cut-border machine, so decoding is one pass with no Zip/offset pre-pass)
//   D  like C, but the new vertex is a dummy that closes a hole (boundary loop); its triangles aren't real
// Holes are closed with one dummy vertex each so every mesh is closed; islands are coded one after another.
// Meshes Edgebreaker can't code this way (non-manifold edges or vertices, inconsistent winding, handles/genus
// > 0) fall back to delta-coded indices (Connectivity::raw), so every mesh round-trips.
#include <array>
#include <cstdint>
#include <vector>

namespace eb {

enum Op : uint8_t { C = 0, L = 1, R = 2, E = 3, S = 4, D = 5 };

struct Connectivity {
  bool raw{};                      // fallback: tris stored as indices
  uint32_t nVerts{};               // decoded vertices, including dummies
  std::vector<uint8_t> ops;        // one per triangle except each island's first
  std::vector<uint32_t> offsets;   // one per S: steps along the loop from the gate's end to the tip
  std::vector<uint32_t> rawTris;   // raw only
};

// What the decoder rebuilds: triangles over decoded vertex ids (dummy triangles included), which ids are
// dummies, and for each vertex the triangle it was predicted from: (u, v, o) where the new vertex w closes
// triangle (v, u, w) across gate u->v and o is the far vertex of the triangle already on the other side.
// -1 where there is none (the first triangle of an island).
struct Decoded {
  uint32_t nVerts{};
  std::vector<uint32_t> tris;
  std::vector<uint8_t> dummy;
  std::vector<std::array<int32_t, 3>> pred;
};

// Encoding is offline only (tools/codec/EdgebreakerEncode.h); the engine just decodes.
auto decode( const Connectivity& conn, Decoded& out ) -> bool;

}  // namespace eb
