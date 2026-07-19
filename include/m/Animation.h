#include "c/Collision.h"
#include <vector>
#include <string>
#include <unordered_map>

struct AnimFrame {
  unsigned duration{};
  Rect srcRect{};
  CollRect collisionRect{};
};

enum AnimType { LOOP, ONE_SHOT, PINGPONG };

struct AnimStrip {
  unsigned nFrames{};
  AnimType type{};
  std::vector<AnimFrame> frames{};
};

using Animation = std::unordered_map<std::string, AnimStrip>;
