#pragma once
// Polled input for real-time games (3D): which keys are held right now, the mouse's motion this frame, mouse
// buttons, and the first gamepad (SDL3 gamepad API). The pixel renderers fill it in pollInput(); ASCII only gets
// scripted input (terminals report presses, not holds). Lua reads it through the input* FFI functions
// (scripts/Input3D.lua maps actions onto it). The key-press events to onInput() are unchanged.
#include "c/Input.h"
#include <array>

struct InputState {
  Bitset held{};             // LogicalKey bits held this frame
  Bitset pressed{};          // went down this frame
  float mouseDx{}, mouseDy{}, wheel{};  // pixels (relative mode) this frame
  unsigned mouseButtons{};   // bit 0 left, 1 right, 2 middle
  unsigned mousePressed{};   // went down this frame
  std::array<float, 6> axes{};  // left x/y, right x/y, left/right trigger: -1..1 (triggers 0..1)
  unsigned padButtons{};     // SDL_GamepadButton bits
  unsigned padPressed{};
  bool padConnected{};
  bool mouseCaptured{};
  void beginFrame() {
    pressed.reset();
    mouseDx = mouseDy = wheel = 0;
    mousePressed = 0;
    padPressed = 0;
  }
};

auto inputState() -> InputState&;
