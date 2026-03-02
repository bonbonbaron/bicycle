#include <bitset>
#include <cstdint>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>
#include <string>
#include <array>

// ────────────────────────────────────────────────
//  Logical keys – our unified, cross-platform key namespace
//  Based on USB HID Usage IDs (Keyboard page 0x07)
// ────────────────────────────────────────────────

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
using KeyState = std::bitset<LogicalKey::COUNT>;

// ────────────────────────────────────────────────
//  Main listener class
// ────────────────────────────────────────────────

class Input {
public:
    Input() = default;

    KeyState get_pressed() const;
    void wait_for_any_change(const KeyState& interesting = ~KeyState(0));

private:
    KeyState                pressed_;
    std::atomic<bool>       running_{true};
    std::thread             thread_;

    void background_loop();

    // Platform-specific
    void platform_specific_init();
    void platform_specific_cleanup();
    void platform_specific_listen_loop();

    // Called from platform code – converts native code → LogicalKey
    void platform_handle_native(int native_code, bool is_down);
};  // class Input

// ────────────────────────────────────────────────
// Platform implementations (mapping native → LogicalKey)
// ────────────────────────────────────────────────

#include <linux/input.h>   // KEY_A, KEY_LEFTCTRL, etc.

// Linux evdev / input-event-codes.h uses its own set (often close to USB HID)
LogicalKey linux_evdev_to_logical(int code) {
    switch (code)
    {
        case KEY_A: return LogicalKey::A;
        case KEY_B: return LogicalKey::B;
        // ... continue for most letters, numbers

        case KEY_ENTER:     return LogicalKey::Enter;
        case KEY_ESC:       return LogicalKey::Escape;
        case KEY_BACKSPACE: return LogicalKey::Backspace;
        case KEY_TAB:       return LogicalKey::Tab;
        case KEY_SPACE:     return LogicalKey::Space;

        case KEY_LEFTCTRL:  return LogicalKey::LeftCtrl;
        case KEY_RIGHTCTRL: return LogicalKey::RightCtrl;
        case KEY_LEFTSHIFT: return LogicalKey::LeftShift;
        case KEY_RIGHTSHIFT:return LogicalKey::RightShift;
        case KEY_LEFTALT:   return LogicalKey::LeftAlt;
        case KEY_RIGHTALT:  return LogicalKey::RightAlt;
        case KEY_LEFTMETA:  return LogicalKey::LeftMeta;
        case KEY_RIGHTMETA: return LogicalKey::RightMeta;

        case KEY_LEFT:      return LogicalKey::LeftArrow;
        case KEY_RIGHT:     return LogicalKey::RightArrow;
        case KEY_DOWN:      return LogicalKey::DownArrow;
        case KEY_UP:        return LogicalKey::UpArrow;

        case KEY_F1: return LogicalKey::F1;   // ... F12

        default: return LogicalKey::COUNT;
    }
}  // LogicalKey converter

void Input::platform_handle_native(int code, bool down) {
    LogicalKey lk = linux_evdev_to_logical(code);
    if (lk == LogicalKey::COUNT) {
      return;
    }

    switch (code) {
        case kVK_ANSI_A:      return LogicalKey::A;
        case kVK_ANSI_B:      return LogicalKey::B;
        case kVK_ANSI_C:      return LogicalKey::C;
        case kVK_ANSI_D:      return LogicalKey::D;
        case kVK_ANSI_E:      return LogicalKey::E;
        case kVK_ANSI_F:      return LogicalKey::F;
        case kVK_ANSI_G:      return LogicalKey::G;
        case kVK_ANSI_H:      return LogicalKey::H;
        case kVK_ANSI_I:      return LogicalKey::I;
        case kVK_ANSI_J:      return LogicalKey::J;
        case kVK_ANSI_K:      return LogicalKey::K;
        case kVK_ANSI_L:      return LogicalKey::L;
        case kVK_ANSI_M:      return LogicalKey::M;
        case kVK_ANSI_N:      return LogicalKey::N;
        case kVK_ANSI_O:      return LogicalKey::O;
        case kVK_ANSI_P:      return LogicalKey::P;
        case kVK_ANSI_Q:      return LogicalKey::Q;
        case kVK_ANSI_R:      return LogicalKey::R;
        case kVK_ANSI_S:      return LogicalKey::S;
        case kVK_ANSI_T:      return LogicalKey::T;
        case kVK_ANSI_U:      return LogicalKey::U;
        case kVK_ANSI_V:      return LogicalKey::V;
        case kVK_ANSI_W:      return LogicalKey::W;
        case kVK_ANSI_X:      return LogicalKey::X;
        case kVK_ANSI_Y:      return LogicalKey::Y;
        case kVK_ANSI_Z:      return LogicalKey::Z;
        // ... continue for C–Z
        case kVK_ANSI_0:      return LogicalKey::Key0;
        case kVK_ANSI_1:      return LogicalKey::Key1;
        case kVK_ANSI_2:      return LogicalKey::Key2;
        case kVK_ANSI_3:      return LogicalKey::Key3;
        case kVK_ANSI_4:      return LogicalKey::Key4;
        case kVK_ANSI_5:      return LogicalKey::Key5;
        case kVK_ANSI_6:      return LogicalKey::Key6;
        case kVK_ANSI_7:      return LogicalKey::Key7;
        case kVK_ANSI_8:      return LogicalKey::Key8;
        case kVK_ANSI_9:      return LogicalKey::Key9;

        case kVK_Return:      return LogicalKey::Enter;
        case kVK_Escape:      return LogicalKey::Escape;
        case kVK_Delete:      return LogicalKey::Backspace;
        case kVK_Tab:         return LogicalKey::Tab;
        case kVK_Space:       return LogicalKey::Space;

        case kVK_Control:     return LogicalKey::LeftCtrl;   // both left/right map here usually
        case kVK_Shift:       return LogicalKey::LeftShift;

        case kVK_RightControl:return LogicalKey::RightCtrl;
        case kVK_RightShift:  return LogicalKey::RightShift;

        default: return LogicalKey::COUNT;
    }
}

// ────────────────────────────────────────────────
// Shared code (constructor, destructor, get_pressed, etc.) same as before
// Just replace the old platform_handle_key(int, bool) with the new one above
// ────────────────────────────────────────────────

// Example usage

int main() {
    Input input;
    while (true) {
        input.wait_for_any_change();
        auto state = input.get_pressed();
    }
    std::cout << "\nDone.\n";
    return 0;
}
