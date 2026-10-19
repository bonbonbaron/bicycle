// Lua FFI: the current scene's HUD layer (scripts/Hud.lua wraps it): meters and text labels, glued to the screen.
// Each is an entity in the layer; its id is what these take and return (-1: there's no scene to put it on).
#include "LuaApi.h"
#include "v/common/PixelImages.h"
#include <algorithm>

using gfx::Widget;

namespace {
// This scene's widget id, if it's one.
auto widget( int id ) -> Widget* {
  auto scene = lua::currentScene();
  return scene && id >= 0 ? scene->widget( static_cast<Entity>( id ) ) : nullptr;
}

auto add( const Widget& w ) -> int {
  auto scene = lua::currentScene();
  return scene ? static_cast<int>( scene->addWidget( w ) ) : -1;
}
}  // namespace

extern "C" {

// A meter at screen cell (x, y), starting full.
EXPORT_API int addHudBar( const char* label, int max, int len, int x, int y ) {
  Widget w;
  w.kind = Widget::BAR;
  w.text = label ? label : "";
  w.max = w.curr = std::max( 1, max );
  w.len = std::max( 1, len );
  w.x = x;
  w.y = y;
  return add( w );
}

EXPORT_API void setHudBar( int id, int val ) {
  if ( Widget* w = widget( id ) ) w->curr = std::clamp( val, 0, w->max );
}

// Pixel modes: draw meter id from images (asset paths of .bimg files; either may be null or "").
// The frame is drawn as is over the fill, which is cropped to the meter's fraction. ASCII ignores this.
EXPORT_API bool setHudBarSprites( int id, const char* framePath, const char* fillPath ) {
  Widget* w = widget( id );
  if ( !w ) return false;
  w->frameSprite = pixels::loadShared( framePath );
  w->fillSprite = pixels::loadShared( fillPath );
  return ( !framePath || !*framePath || w->frameSprite ) && ( !fillPath || !*fillPath || w->fillSprite );
}

// A text label: x >= 0 a cell, -1 left, -2 right (right-aligned), -3 centered; y >= 0 a row, -1 top, -2 bottom
// (gfx::Widget::Edge).
EXPORT_API int addHudText( const char* text, int x, int y ) {
  Widget w;
  w.kind = Widget::TEXT;
  w.text = text ? text : "";
  w.x = x;
  w.y = y;
  return add( w );
}

EXPORT_API void setHudText( int id, const char* text ) {
  if ( Widget* w = widget( id ) ) w->text = text ? text : "";
}

// Takes a meter or label off the current scene's HUD.
EXPORT_API void removeHudItem( int id ) {
  auto scene = lua::currentScene();
  if ( scene && id >= 0 ) scene->removeWidget( static_cast<Entity>( id ) );
}

}  // extern "C"
