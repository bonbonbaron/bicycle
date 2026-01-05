#pragma once
#include "Node.h"
#include <memory>

struct Node;

struct Edge {
  std::string name;  // e.g. Door
  bool open{true};
  int weight{};
  std::shared_ptr<Node> node{};
};
