#pragma once
// .bmesh: a compressed triangle mesh (tools/pack makes them from Blender's OBJ exports; see scripts/blender).
//   connectivity  Edgebreaker CLERS symbols + S offsets (include/data/Edgebreaker.h)
//   positions     quantized to qbits per axis over the bounding box, predicted with the parallelogram rule
//                 (w ~ u + v - o across the gate) in Edgebreaker order; residuals zigzag-varint coded
//   normals       per corner, octahedral, 10 bits per component, delta coded
//   uvs           per corner, 16 bits per component over the UV bounding box, delta coded
//   materials     per triangle (u8) + a table of { name, diffuse RGBA, texture asset }
// Every stream is then deflated (zlib; inflated with tinfl).
//
//   skin          (flag 8) per vertex 4 joint indices + 4 weights (u8), in decoded vertex order: Edgebreaker
//                 renumbers vertices, so the encoder writes them through its decoded-to-input mapping
// Layout: "BMSH" u32 version(1) u32 nVerts u32 nTris u8 flags(1 raw connectivity, 2 normals, 4 uvs, 8 skin) u8 qbits
//         f32 bmin[3] f32 bmax[3] f32 uvmin[2] f32 uvmax[2] u32 nDecodedVerts (incl. hole dummies)
//         u16 nMaterials { u16 len name, f32 rgba[4], u16 len texture }
//         7 streams { u32 rawLen u32 compLen bytes }: ops, offsets, rawTris, positions, normals, uvs, materials
//         (+ an 8th, skin, with flag 8)
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "data/Mem.h"

struct MeshMaterial {
  std::string name;
  std::array<float, 4> rgba{ 0.8f, 0.8f, 0.8f, 1.0f };
  std::string texture;  // asset path relative to the game folder (a .bimg), or empty
};

struct MeshData {
  // Geometry lives in the GRAPHIC pool.
  mem::Vector<float, mem::GRAPHIC> pos;       // 3 per vertex
  mem::Vector<uint32_t, mem::GRAPHIC> tris;   // 3 per triangle (counter-clockwise)
  mem::Vector<float, mem::GRAPHIC> normals;   // 3 per corner (3 corners per triangle), or empty
  mem::Vector<float, mem::GRAPHIC> uvs;       // 2 per corner, or empty
  mem::Vector<uint8_t, mem::GRAPHIC> triMat;  // 1 per triangle
  // Skinned meshes: 4 joint indices and 4 weights (summing to 255) per vertex, or empty.
  mem::Vector<uint8_t, mem::GRAPHIC> joints, weights;
  // readObj only: the OBJ "v" index each vertex came from (to apply a .bskin's per-OBJ-vertex weights).
  mem::Vector<uint32_t, mem::GRAPHIC> srcVertex;
  std::vector<MeshMaterial> materials;
  auto nVerts() const -> size_t { return pos.size() / 3; }
  auto nTris() const -> size_t { return tris.size() / 3; }
};

namespace mesh {
constexpr int DEFAULT_QBITS = 14;
auto decode( std::string_view bmesh ) -> std::optional<MeshData>;
// Octahedral normal coding shared by the encoder and decoder (bits per component).
constexpr int NORMAL_BITS = 10;
auto octEncode( const float n[3] ) -> std::array<int32_t, 2>;
void octDecode( int32_t a, int32_t b, float n[3] );
// Position prediction (see above) over quantized positions q (3 per vertex); dummy[] marks hole vertices.
auto predict( const std::vector<int32_t>& q, const std::vector<uint8_t>& known, std::array<int32_t, 3> ctx,
              int32_t last, int axis, int qbits ) -> int32_t;
}  // namespace mesh
