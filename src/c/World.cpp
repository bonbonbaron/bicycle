#include "m/World.h"

World() {
  initialize_all() {
}

static auto getInstance() -> World& {
  static World world;
  return world;
}

