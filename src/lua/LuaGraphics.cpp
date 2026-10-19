// Lua FFI: what entities look like: ASCII images, pixel images (see v/common/PixelImages.h) and 3D models.
#include "LuaApi.h"
#include "v/common/PixelImages.h"
#include "v/common/Renderer.h"
#include "c/Kinematics.h"

namespace {
// The entity's image, and its box sized to it.
void setImage( Entity entity, Image img ) {
  World::set<Image>( entity, std::move( img ) );
  World::get<Box>( entity ).size = World::get<Image>( entity ).size;
  Kinematics::getInstance().moved( entity );  // a new size: collision checks
}
}  // namespace

extern "C" {

// Which renderer this build has: 0 ASCII, 1 2D, 2 3D (Lua picks ASCII art or pixels by it).
EXPORT_API int renderMode() {
  return static_cast<int>( gfx::renderer().mode() );
}

EXPORT_API void newImage( Entity entity, const char* imgStr ) {
  setImage( entity, Image( imgStr ) );
}

// A pixel image from .bimg layers (newline-separated paths, bottom layer first; frame strips of one size, from
// scripts/aseprite/sprite.lua's pixel export). Returns false if one's missing.
EXPORT_API bool newPixelImage( Entity entity, const char* paths ) {
  auto img = pixels::sprite( pixels::splitLines( paths ) );
  if ( !img ) return false;
  setImage( entity, pixels::asImage( img ) );
  return true;
}

// The same, recolored (genome palette = "<name>_palette": "i:rgba,..." entries replaced).
EXPORT_API bool newPixelImagePal( Entity entity, const char* paths, const char* palette ) {
  auto img = pixels::sprite( pixels::splitLines( paths ), palette ? palette : "" );
  if ( !img ) return false;
  setImage( entity, pixels::asImage( img ) );
  return true;
}

// 3D mode: draw the entity as this mesh (an asset path of a .bmesh). False in other modes or if it's missing.
EXPORT_API bool setModel( Entity entity, const char* bmeshPath ) {
  return gfx::renderer().setModel( entity, bmeshPath );
}

// 3D mode: the action a rigged model plays (Animation:set maps strip tags to actions).
EXPORT_API void setModelAction( Entity entity, const char* action ) {
  gfx::renderer().setModelAction( entity, action );
}

// 3D mode: static scenery (ground, props) exported from Blender with its world transform baked in.
EXPORT_API bool addStaticModel( const char* bmeshPath ) {
  return gfx::renderer().addStaticModel( bmeshPath );
}

}  // extern "C"
