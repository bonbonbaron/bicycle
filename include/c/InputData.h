#pragma once
#include <bitset>

// A bunch of stuff used by both controller types (physical keyboard and SSH key events)
enum class LogicalKey : uint8_t {
    // Letters (HID 0x04–0x1D)  <-- these will be treated as case-insensitive
    A ,   B ,   C ,   D ,   E ,
    F ,   G ,   H ,   I ,   J ,
    K ,   L ,   M ,   N ,   O ,
    P ,   Q ,   R ,   S ,   T ,
    U ,   V ,   W ,   X ,   Y ,
    Z ,

    // Numbers (HID 0x1E–0x27)
    Key1 , Key2 , Key3 , Key4 , Key5 ,
    Key6 , Key7 , Key8 , Key9 , Key0 ,

    // Common punctuation / symbols
    Enter     ,
    Escape    ,
    Tab       ,
    Space     ,

    // Modifiers
    LeftCtrl ,
    LeftShift ,
    LeftAlt   ,     // Left Option on macOS
    LeftMeta  ,     // Left Win / Left Cmd
    RightCtrl ,
    RightShift,
    RightAlt  ,     // Right Option / AltGr
    RightMeta ,     // Right Win / Right Cmd

    COUNT     // ← use this to know how many we defined (not a real key)
};  // enum LogicalKey

// We use these values directly as bit indices
using KeyState = std::bitset<static_cast<long unsigned int>(LogicalKey::COUNT)>;  // C++ is hyper-autistic.

struct InputState {
  KeyState currKeysPressed{};
  KeyState deltaKeysPressed{};
  int millisSinceLast{};  // i don't like making input dependent on Timer.
};

