#include "m/World.h"

World::World() {
  initialize_all();
}

auto World::getInstance() -> World& {
  static World world;
  return world;
}

