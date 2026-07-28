#include "m/World.h"
#include "export.h"

World::World() {
  initialize_all();
}

auto World::getInstance() -> World& {
  static World world;
  return world;
}

extern "C" {
  EXPORT_API
  LWorldArr<Box> World_getBoxes() { 
    return World_getArr<Box>(); 
  }

  EXPORT_API
  LWorldArr<CollBox> World_getCollBoxes() { 
    return World_getArr<CollBox>(); 
  }

  EXPORT_API
  LWorldArr<Image> World_getImages() { 
    return World_getArr<Image>(); 
  }

  EXPORT_API
  LWorldArr<Rect> World_getRects() { 
    return World_getArr<Rect>(); 
  }
}
