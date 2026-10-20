#pragma once
// Shared by the Lua-facing C exports (src/lua/Lua*.cpp; scripts/bicycle.lua declares them for LuaJIT's FFI):
// the window on top of the stack, and the scene it is, if it is one.
#include "c/WindowManager.h"
#include "v/common/Scene.h"
#include "export.h"
#include "m/World.h"
#include <memory>

namespace lua {

// The top window as a T, or null if there's none or it's something else.
template <class T>
auto topWindowAs() -> std::shared_ptr<T> {
  return std::dynamic_pointer_cast<T>( WindowManager::getInstance().back() );
}

// The scene being played (null while a dialogue or menu is on top of it, or before there is one).
inline auto currentScene() -> std::shared_ptr<Scene> { return topWindowAs<Scene>(); }

// A velocity in fixed-point cells per frame (256 = one cell per frame).
inline auto velocity( int vx, int vy ) -> Position {
  Position v{};
  v.x = vx;
  v.y = vy;
  return v;
}
// A position already in fixed point (no shift, unlike Position( x, y )).
inline auto fixedPos( int x, int y ) -> Position { return velocity( x, y ); }

// These functions are essentially protectors, ensuring that if you change definition of Position, this breaks as an alarm.
inline auto velX( int vx ) -> decltype(Position::x) { return vx; }
inline auto velY( int vy ) -> decltype(Position::y) { return vy; }
inline auto velZ( int vz ) -> decltype(Position::z) { return vz; }

}  // namespace lua
