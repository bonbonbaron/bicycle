#pragma once
// .bskin: a character's skeleton and animation actions (scripts/blender/export.py writes them next to the OBJ).
//   joints   name, parent (-1 for roots; parents come first), inverse bind matrix (mesh space, Y up)
//   vertices (loose files only) per OBJ vertex: 4 joint indices + 4 weights (u8, summing to 255), the 4 largest
//            influences. Packed meshes carry these in the .bmesh instead, in Edgebreaker's vertex order.
//   actions  name, fps, frames; per frame per joint the local transform (translation + rotation quaternion)
//
// Layout: "BSKN" u8 version(1) u8 flags(1 quantized, 2 has vertices) u16 nJoints u16 nActions u32 nVerts
//         nJoints  { u8 len name, i16 parent, f32 inverseBind[16] (column major) }
//         flags&2: nVerts { u8 joints[4], u8 weights[4] }
//         nActions { u8 len name, f32 fps, u16 nFrames, keys }
//   keys, float (as exported, dev):  nFrames x nJoints { f32 t[3], f32 q[4] (x, y, z, w) }
//   keys, quantized (bicycle-pack):   f32 tScale, then nFrames x nJoints { i16 t[3] (t / tScale * 32767),
//                                     u8 largest-component index, i16 the other 3 (x sqrt2 * 32767) }, each frame
//                                     stored as the difference from the previous frame (zigzag varints) so the
//                                     pack's deflate squeezes it.
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "data/Mem.h"

struct SkinJoint {
  std::string name;
  int16_t parent{-1};
  std::array<float, 16> inverseBind{};
};

struct SkinAction {
  std::string name;
  float fps{24};
  uint32_t nFrames{};
  mem::Vector<float, mem::GRAPHIC> t;  // nFrames * nJoints * 3
  mem::Vector<float, mem::GRAPHIC> q;  // nFrames * nJoints * 4
};

struct Skin {
  std::vector<SkinJoint> joints;
  std::vector<SkinAction> actions;
  mem::Vector<uint8_t, mem::GRAPHIC> vJoints, vWeights;  // 4 per vertex; loose files only
  auto action( std::string_view name ) const -> const SkinAction*;
};

namespace skin {
constexpr int MAX_JOINTS = 32;  // the skinning shader's uniform array
auto decode( std::string_view bytes ) -> std::optional<Skin>;
auto encode( const Skin& s, bool quantized, bool withVertices ) -> std::string;
// Skinning matrices (joint global transform x inverse bind), 16 floats each, for an action at time seconds
// (looping, linear interpolation between frames; nlerp for rotations).
void pose( const Skin& s, const SkinAction& a, float seconds, float* out );
// The rest pose (identity skinning) for joints without an action.
void restPose( const Skin& s, float* out );
}  // namespace skin
