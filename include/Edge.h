#pragma once
#include "Node.h"
#include <memory>

struct Node;

struct Edge {
  bool open{true};
  int weight{};
  std::shared_ptr<Node> node{};
};
