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
  LWorldArr<Rect> World_getRects() { 
    return World_getArr<Rect>(); 
  }

  EXPORT_API
  LWorldArr<CollRect> World_getCollRects() { 
    return World_getArr<CollRect>(); 
  }

  EXPORT_API
  LWorldArr<Image> World_getImages() { 
    return World_getArr<Image>(); 
  }

  EXPORT_API
  LWorldArr<Velocity> World_getVels() { 
    return World_getArr<Velocity>(); 
  }

  EXPORT_API
  LWorldArr<Animation> World_getAnims() { 
    return World_getArr<Animation>(); 
  }
}
