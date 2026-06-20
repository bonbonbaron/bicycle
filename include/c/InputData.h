#pragma once
#include <bitset>

// A bunch of stuff used by both controller types (physical keyboard and SSH key events)
  enum class LogicalKey : uint8_t {
      // Letters (HID 0x04–0x1D)  <-- these will be treated as case-insensitive
      A ,
      B ,
      C ,
      D ,
      E ,
      F ,
      G ,
      H ,
      I ,
      J ,
      K ,
      L ,
      M ,
      N ,
      O ,
      P ,
      Q ,
      R ,
      S ,
      T ,
      U ,
      V ,
      W ,
      X ,
      Y ,
      Z ,

      // Numbers (HID 0x1E–0x27)
      Key1 ,
      Key2 ,
      Key3 ,
      Key4 ,
      Key5 ,
      Key6 ,
      Key7 ,
      Key8 ,
      Key9 ,
      Key0 ,

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

using Bitset = std::bitset<static_cast<long unsigned int>(LogicalKey::COUNT)>;

constexpr Bitset getbs( LogicalKey lk ) {
  return Bitset{ 1ULL << static_cast<unsigned>( lk ) };
}


// Letters (HID 0x04–0x1D)  <-- these will be treated as case-insensitive
constexpr auto MASK_A = getbs( LogicalKey::A);
constexpr auto MASK_B = getbs( LogicalKey::B);
constexpr auto MASK_C = getbs( LogicalKey::C);
constexpr auto MASK_D = getbs( LogicalKey::D);
constexpr auto MASK_E = getbs( LogicalKey::E);
constexpr auto MASK_F = getbs( LogicalKey::F);
constexpr auto MASK_G = getbs( LogicalKey::G);
constexpr auto MASK_H = getbs( LogicalKey::H);
constexpr auto MASK_I = getbs( LogicalKey::I);
constexpr auto MASK_J = getbs( LogicalKey::J);
constexpr auto MASK_K = getbs( LogicalKey::K);
constexpr auto MASK_L = getbs( LogicalKey::L);
constexpr auto MASK_M = getbs( LogicalKey::M);
constexpr auto MASK_N = getbs( LogicalKey::N);
constexpr auto MASK_O = getbs( LogicalKey::O);
constexpr auto MASK_P = getbs( LogicalKey::P);
constexpr auto MASK_Q = getbs( LogicalKey::Q);
constexpr auto MASK_R = getbs( LogicalKey::R);
constexpr auto MASK_S = getbs( LogicalKey::S);
constexpr auto MASK_T = getbs( LogicalKey::T);
constexpr auto MASK_U = getbs( LogicalKey::U);
constexpr auto MASK_V = getbs( LogicalKey::V);
constexpr auto MASK_W = getbs( LogicalKey::W);
constexpr auto MASK_X = getbs( LogicalKey::X);
constexpr auto MASK_Y = getbs( LogicalKey::Y);
constexpr auto MASK_Z = getbs( LogicalKey::Z);

// Numbers (HID 0x1E–0x27)

constexpr auto MASK_1 = getbs( LogicalKey::Key1);
constexpr auto MASK_2 = getbs( LogicalKey::Key2);
constexpr auto MASK_3 = getbs( LogicalKey::Key3);
constexpr auto MASK_4 = getbs( LogicalKey::Key4);
constexpr auto MASK_5 = getbs( LogicalKey::Key5);
constexpr auto MASK_6 = getbs( LogicalKey::Key6);
constexpr auto MASK_7 = getbs( LogicalKey::Key7);
constexpr auto MASK_8 = getbs( LogicalKey::Key8);
constexpr auto MASK_9 = getbs( LogicalKey::Key9);
constexpr auto MASK_0 = getbs( LogicalKey::Key0);

// Common punctuation / symbols
constexpr auto MASK_ENTER  = getbs( LogicalKey::Enter);
constexpr auto MASK_ESCAPE = getbs( LogicalKey::Escape);
constexpr auto MASK_TAB    = getbs( LogicalKey::Tab);
constexpr auto MASK_SPACE  = getbs( LogicalKey::Space);

// Modifiers
constexpr auto MASK_LCTRL  = getbs( LogicalKey::LeftCtrl);
constexpr auto MASK_LSHIFT = getbs( LogicalKey::LeftShift);
constexpr auto MASK_LALT   = getbs( LogicalKey::LeftAlt);
constexpr auto MASK_LMETA  = getbs( LogicalKey::LeftMeta);
constexpr auto MASK_RCTRL  = getbs( LogicalKey::RightCtrl);
constexpr auto MASK_RSHIFT = getbs( LogicalKey::RightShift);
constexpr auto MASK_RMETA  = getbs( LogicalKey::RightMeta);

struct InputState {
  Bitset currKeysPressed{};
  Bitset deltaKeysPressed{};
  int millisSinceLast{};  // i don't like making input dependent on Timer.
};

