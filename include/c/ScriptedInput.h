#pragma once
// Input from the environment instead of a player, for headless captures and tests (any mode), sent through the
// same path as real keys and mice:
//   BICYCLE_SCRIPTED_INPUT="d@30-60/3,a@70-90/2": press D every 3rd frame from frame 30 to 60, then A every 2nd
//     from 70 to 90 (letters and space). "/1" (or no step) holds the key from the first frame to the last; a step
//     taps: each occurrence presses and the next frame releases.
//   BICYCLE_SCRIPTED_MOUSE="12:0@30-60;L@80-80;R@90-95": mouse motion (dx:dy pixels per frame) or a held button
//     (L/R/M) over a frame range, into the polled input state (c/InputState.h), like a real mouse.
// Scripted keys also count as held keys on their frames.
#include "c/Input.h"
#include <vector>

class ScriptedInput {
  public:
    static auto fromEnv() -> ScriptedInput;
    void play( long frame );  // this frame's scripted events

  private:
    struct Key { LogicalKey key; long from, to, every; bool down; };
    struct Mouse { float dx, dy; int button; long from, to; };  // button -1: motion
    std::vector<Key> _keys;
    std::vector<Mouse> _mouse;
    unsigned _buttons{};  // scripted mouse buttons down last frame
};
