#pragma once
#include <bitset>
#include <cstdint>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <fcntl.h>
#include <unistd.h> // For read/write
#include <poll.h>
#include <linux/input.h>
#include <string_view>
#include <libevdev/libevdev.h>
#include <dirent.h>
#include <sys/types.h>

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

// ────────────────────────────────────────────────
//  Main listener class
// ────────────────────────────────────────────────
class Input {
  public:
    static auto getInstance() -> Input&;
    void _listen();
    static void listen();
  private:
    Input();
    Input(const Input&) = delete;
    Input operator=(const Input&) = delete;
    Input(const Input&&) = delete;
    Input operator=(const Input&&) = delete;

    auto convertCodeToLogicalInt(int code) -> LogicalKey;

    // Most importantly...
    KeyState _keyState{};

    // To support not knowing where keyboard lives...
    struct Device {
      int fd = -1;
      struct libevdev* dev = nullptr;
      std::string path;
    };

    std::vector<Device> _devices;

    void discoverPotentialKeyboards();
    // void close_all();
    void reopenIfNeeded();  // optional: call every ~5s for hot-switch support

};  // class Input

