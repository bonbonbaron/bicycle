#include "c/InputState.h"
#include "v/common/Renderer.h"
#include "export.h"

auto inputState() -> InputState& {
  static InputState s;
  return s;
}

extern "C" {
  EXPORT_API bool inputHeld( int key ) { return key > 0 && key < static_cast<int>( LogicalKey::COUNT ) && inputState().held.test( key ); }
  EXPORT_API bool inputPressed( int key ) { return key > 0 && key < static_cast<int>( LogicalKey::COUNT ) && inputState().pressed.test( key ); }
  EXPORT_API float inputMouseDx() { return inputState().mouseDx; }
  EXPORT_API float inputMouseDy() { return inputState().mouseDy; }
  EXPORT_API float inputWheel() { return inputState().wheel; }
  EXPORT_API bool inputMouseButton( int b ) { return b >= 0 && b < 8 && ( inputState().mouseButtons >> b & 1u ); }
  EXPORT_API bool inputMousePressed( int b ) { return b >= 0 && b < 8 && ( inputState().mousePressed >> b & 1u ); }
  EXPORT_API float inputPadAxis( int a ) { return a >= 0 && a < 6 ? inputState().axes[a] : 0.f; }
  EXPORT_API bool inputPadButton( int b ) { return b >= 0 && b < 32 && ( inputState().padButtons >> b & 1u ); }
  EXPORT_API bool inputPadPressed( int b ) { return b >= 0 && b < 32 && ( inputState().padPressed >> b & 1u ); }
  EXPORT_API bool inputPadConnected() { return inputState().padConnected; }
  // Relative mouse mode (mouse look): the cursor hides and the mouse reports motion only.
  EXPORT_API void inputSetMouseCapture( bool on ) { gfx::renderer().setMouseCapture( on ); }
  EXPORT_API bool inputMouseCaptured() { return inputState().mouseCaptured; }
  // Opens SDL's gamepad subsystem (on demand: it scans devices, which a game without a pad needn't do).
  EXPORT_API void inputEnableGamepad() { gfx::renderer().enableGamepad(); }
}
