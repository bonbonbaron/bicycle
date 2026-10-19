// Lua FFI: the current scene's HUD (scripts/Hud.lua): meters and text labels glued to the screen.
#include "LuaApi.h"
#include "v/common/PixelImages.h"

using lua::currentScene;

extern "C" {

// A meter glued to the current scene at screen cell (x, y), starting full; returns its id (-1 if no scene).
EXPORT_API int addHudBar( const char* label, int max, int len, int x, int y ) {
  auto scene = currentScene();
  return scene ? scene->addHudBar( label, max, len, x, y ) : -1;
}

EXPORT_API void setHudBar( int id, int val ) {
  auto scene = currentScene();
  if ( Bar* bar = scene ? scene->getHudBar( id ) : nullptr ) bar->setValue( val );
}

// Pixel modes: draw HUD bar id from images (asset paths of .bimg files; either may be null or "").
// The frame is drawn as is over the fill, which is cropped to the bar's fraction. ASCII ignores this.
EXPORT_API bool setHudBarSprites( int id, const char* framePath, const char* fillPath ) {
  auto scene = currentScene();
  Bar* bar = scene ? scene->getHudBar( id ) : nullptr;
  if ( !bar ) return false;
  auto frame = pixels::loadShared( framePath ), fill = pixels::loadShared( fillPath );
  bar->setSprites( frame, fill );
  return ( !framePath || !*framePath || frame ) && ( !fillPath || !*fillPath || fill );
}

// A text label on the current scene (see Scene::addHudText for x and y); returns its id (-1 if no scene).
EXPORT_API int addHudText( const char* text, int x, int y ) {
  auto scene = currentScene();
  return scene ? scene->addHudText( text ? text : "", x, y ) : -1;
}

EXPORT_API void setHudText( int id, const char* text ) {
  if ( auto scene = currentScene() ) scene->setHudText( id, text ? text : "" );
}

}  // extern "C"
