#pragma once
#include <vector>
#include <string>
#include <unordered_map>

struct AnimFrame {
  unsigned duration{};
  Box srcBox{};
  CollBox collisionBox{};
};

enum AnimType { LOOP, ONE_SHOT, PINGPONG };

struct AnimStrip {
  unsigned nFrames{};
  AnimType type{};
  std::vector<AnimFrame> frames{};
};

struct Animation : std::unordered_map<std::string, AnimStrip> {
  Animation() = default;
};
