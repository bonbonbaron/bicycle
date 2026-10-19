// Lua FFI: the window stack (dialogues, menus, popping) and the screen's size.
#include "LuaApi.h"
#include "bicycle.h"
#include "v/common/Dialogue.h"
#include "v/common/TextMenu.h"
#include "v/common/Renderer.h"

using lua::topWindowAs;

extern "C" {

EXPORT_API Entity pushDialogue( const char* text, int x, int y, int w, int h ) {
  return bicycle::push<Dialogue>( text, x, y, w, h );
}

// Whether the top window is a dialogue whose reader pressed E or Space on its last page.
EXPORT_API bool isDialogueDone() {
  auto dialogue = topWindowAs<Dialogue>();
  return dialogue && dialogue->isDone();
}

EXPORT_API Entity pushTextMenu( int x, int y, int w, int h ) {
  return bicycle::push<TextMenu>( x, y, w, h );
}

EXPORT_API void addItem( const char* text ) {
  if ( auto menu = topWindowAs<Menu>() ) menu->addItem( text );
}

// The top menu's selected item (0 if the top window isn't a menu).
EXPORT_API unsigned getSelection() {
  auto menu = topWindowAs<Menu>();
  return menu ? menu->getSelection() : 0;
}

// The window on top: whether it's a scene, and its id (Scene.goto closes dialogues and menus over the scene).
EXPORT_API bool topIsScene() {
  return lua::currentScene() != nullptr;
}

EXPORT_API Entity topWindowId() {
  auto w = WindowManager::getInstance().back();
  return w ? w->getId() : 0;
}

EXPORT_API Entity popWindow() {
  return bicycle::pop();
}

EXPORT_API int getScreenWidth() { return gfx::cols(); }
EXPORT_API int getScreenHeight() { return gfx::lines(); }

}  // extern "C"
