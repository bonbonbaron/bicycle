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

// Letters (HID 0x04–0x1D)  <-- these will be treated as case-insensitive
constexpr auto MASK_A = Bitset{ static_cast<unsigned>( LogicalKey::A ) };
constexpr auto MASK_B = Bitset{ static_cast<unsigned>( LogicalKey::B ) };
constexpr auto MASK_C = Bitset{ static_cast<unsigned>( LogicalKey::C ) };
constexpr auto MASK_D = Bitset{ static_cast<unsigned>( LogicalKey::D ) };
constexpr auto MASK_E = Bitset{ static_cast<unsigned>( LogicalKey::E ) };
constexpr auto MASK_F = Bitset{ static_cast<unsigned>( LogicalKey::F ) };
constexpr auto MASK_G = Bitset{ static_cast<unsigned>( LogicalKey::G ) };
constexpr auto MASK_H = Bitset{ static_cast<unsigned>( LogicalKey::H ) };
constexpr auto MASK_I = Bitset{ static_cast<unsigned>( LogicalKey::I ) };
constexpr auto MASK_J = Bitset{ static_cast<unsigned>( LogicalKey::J ) };
constexpr auto MASK_K = Bitset{ static_cast<unsigned>( LogicalKey::K ) };
constexpr auto MASK_L = Bitset{ static_cast<unsigned>( LogicalKey::L ) };
constexpr auto MASK_M = Bitset{ static_cast<unsigned>( LogicalKey::M ) };
constexpr auto MASK_N = Bitset{ static_cast<unsigned>( LogicalKey::N ) };
constexpr auto MASK_O = Bitset{ static_cast<unsigned>( LogicalKey::O ) };
constexpr auto MASK_P = Bitset{ static_cast<unsigned>( LogicalKey::P ) };
constexpr auto MASK_Q = Bitset{ static_cast<unsigned>( LogicalKey::Q ) };
constexpr auto MASK_R = Bitset{ static_cast<unsigned>( LogicalKey::R ) };
constexpr auto MASK_S = Bitset{ static_cast<unsigned>( LogicalKey::S ) };
constexpr auto MASK_T = Bitset{ static_cast<unsigned>( LogicalKey::T ) };
constexpr auto MASK_U = Bitset{ static_cast<unsigned>( LogicalKey::U ) };
constexpr auto MASK_V = Bitset{ static_cast<unsigned>( LogicalKey::V ) };
constexpr auto MASK_W = Bitset{ static_cast<unsigned>( LogicalKey::W ) };
constexpr auto MASK_X = Bitset{ static_cast<unsigned>( LogicalKey::X ) };
constexpr auto MASK_Y = Bitset{ static_cast<unsigned>( LogicalKey::Y ) };
constexpr auto MASK_Z = Bitset{ static_cast<unsigned>( LogicalKey::Z ) };

// Numbers (HID 0x1E–0x27)

constexpr auto MASK_1 = Bitset{ static_cast<unsigned>( LogicalKey::Key1 ) };
constexpr auto MASK_2 = Bitset{ static_cast<unsigned>( LogicalKey::Key2 ) };
constexpr auto MASK_3 = Bitset{ static_cast<unsigned>( LogicalKey::Key3 ) };
constexpr auto MASK_4 = Bitset{ static_cast<unsigned>( LogicalKey::Key4 ) };
constexpr auto MASK_5 = Bitset{ static_cast<unsigned>( LogicalKey::Key5 ) };
constexpr auto MASK_6 = Bitset{ static_cast<unsigned>( LogicalKey::Key6 ) };
constexpr auto MASK_7 = Bitset{ static_cast<unsigned>( LogicalKey::Key7 ) };
constexpr auto MASK_8 = Bitset{ static_cast<unsigned>( LogicalKey::Key8 ) };
constexpr auto MASK_9 = Bitset{ static_cast<unsigned>( LogicalKey::Key9 ) };
constexpr auto MASK_0 = Bitset{ static_cast<unsigned>( LogicalKey::Key0 ) };

// Common punctuation / symbols
constexpr auto MASK_ENTER  = Bitset{ static_cast<unsigned>( LogicalKey::Enter  ) };
constexpr auto MASK_ESCAPE = Bitset{ static_cast<unsigned>( LogicalKey::Escape ) };
constexpr auto MASK_TAB    = Bitset{ static_cast<unsigned>( LogicalKey::Tab    ) };
constexpr auto MASK_SPACE  = Bitset{ static_cast<unsigned>( LogicalKey::Space  ) };

// Modifiers
constexpr auto MASK_LCTRL  = Bitset{ static_cast<unsigned>( LogicalKey::LeftCtrl   ) };
constexpr auto MASK_LSHIFT = Bitset{ static_cast<unsigned>( LogicalKey::LeftShift  ) };
constexpr auto MASK_LALT   = Bitset{ static_cast<unsigned>( LogicalKey::LeftAlt    ) };
constexpr auto MASK_LMETA  = Bitset{ static_cast<unsigned>( LogicalKey::LeftMeta   ) };
constexpr auto MASK_RCTRL  = Bitset{ static_cast<unsigned>( LogicalKey::RightCtrl  ) };
constexpr auto MASK_RSHIFT = Bitset{ static_cast<unsigned>( LogicalKey::RightShift ) };
constexpr auto MASK_RALT   = Bitset{ static_cast<unsigned>( LogicalKey::RightAlt   ) };
constexpr auto MASK_RMETA  = Bitset{ static_cast<unsigned>( LogicalKey::RightMeta  ) };

struct InputState {
  Bitset currKeysPressed{};
  Bitset deltaKeysPressed{};
  int millisSinceLast{};  // i don't like making input dependent on Timer.
};

