#pragma once
#include <bitset>
#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <string_view>

#include "c/LogicalKey.h"

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <unistd.h>
#  include <poll.h>
#  include <linux/input.h>
#  include <libevdev/libevdev.h>
#  include <dirent.h>
#  include <sys/types.h>
#endif

// ────────────────────────────────────────────────
//  Main listener class
// ────────────────────────────────────────────────
class Input {
public:
    static auto getInstance() -> Input&;
    void listen();   // platform-specific implementation

private:
    Input();
    Input(const Input&) = delete;
    Input operator=(const Input&) = delete;
    Input(const Input&&) = delete;
    Input operator=(const Input&&) = delete;

    auto convertCodeToLogicalInt(int code) -> LogicalKey;

    Input _inputState{};

#if !defined(_WIN32)
    struct Device {
        int fd = -1;
        struct libevdev* dev = nullptr;
        std::string path;
    };
    std::vector<Device> _devices;

    void discoverPotentialKeyboards();
    void reopenIfNeeded();
#endif
};