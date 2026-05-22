#pragma once
#include <bitset>
#include <cstdint>
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

#include "c/LogicalKey.h"
// ────────────────────────────────────────────────
//  Main listener class
// ────────────────────────────────────────────────
class Input {
  public:
    static auto getInstance() -> Input&;
    void listen();   // this is called by the Input instance itself
  private:
    Input();
    Input(const Input&) = delete;
    Input operator=(const Input&) = delete;
    Input(const Input&&) = delete;
    Input operator=(const Input&&) = delete;

    auto convertCodeToLogicalInt(int code) -> LogicalKey;

    // Most importantly...
    InputState _inputState{};

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

