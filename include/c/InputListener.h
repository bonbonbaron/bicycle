#pragma once
#include <bitset>
#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <string_view>

#include "c/Input.h"
#include <iostream>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  undef byte          // resolve the ambiguity with std::byte
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
class InputListener {
public:
    static auto getInstance() -> InputListener&;
    void listen();   // platform-specific implementation

private:
    InputListener();
    InputListener(const InputListener&) = delete;
    InputListener operator=(const InputListener&) = delete;
    InputListener(const InputListener&&) = delete;
    InputListener operator=(const InputListener&&) = delete;

    auto convertCodeToLogicalInt(int code) -> LogicalKey;
    void press(LogicalKey lk, bool down);  // record one key edge in _state.currKeysPressed
    void flush();                          // send every key that changed since last frame to Trigger
    Input _state{};
    bool _useTerminal{};                   // no keyboard device/console of our own: fall back to SshInput (stdin)

#if defined(_WIN32)
    HANDLE _console{ INVALID_HANDLE_VALUE };
#endif

#if !defined(_WIN32)
    struct Device {
        int fd = -1;
        struct libevdev* dev = nullptr;
        std::string path;
    };
    std::vector<Device> _devices;

    void discoverPotentialKeyboards();
#endif
};