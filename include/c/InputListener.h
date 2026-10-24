#pragma once
// The one keyboard listener for every mode and platform. Every key source calls press( key, down ) on an edge of
// a physical key; listen() (once per frame, from the renderer's pollInput) polls the terminal backend if there is
// one and hands the resulting presses and releases to Trigger::sendInput, in order.
//
//   2D / 3D (SDL window, any OS)      PixelRenderer's SDL event pump calls press(); nothing else to poll
//   ASCII, Linux local console / Pi   evdev keyboards (/dev/input/event*, needs the "input" group)
//   ASCII, Linux over SSH or no evdev the terminal (SshInput: raw stdin, kitty key releases or auto key-up)
//   ASCII, Windows                    GetAsyncKeyState while our terminal has focus (multi-key, exact releases);
//                                     CONIN$ records are only drained. ConPTY (VS Code, Windows Terminal) turns
//                                     each key into a down+up pair and repeats only the last key, so it can't be
//                                     the source of held state.
//   Note: over SSH a terminal sends only characters and repeats only the last key held, so without kitty key
//   releases it's one key at a time (no diagonals). That's inherent; a local Pi console uses evdev (multi-key).
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "c/Input.h"

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  undef byte          // resolve the ambiguity with std::byte
#endif

class InputListener {
public:
    static auto getInstance() -> InputListener&;
    void press( LogicalKey key, bool down );  // a physical key (or button) went down or up
    void listen();                            // poll the backend, then send this frame's key events

private:
    InputListener();
    InputListener( const InputListener& ) = delete;
    InputListener& operator=( const InputListener& ) = delete;

    enum class Backend { WINDOW, EVDEV, TERMINAL, CONSOLE, ASYNC_KEYS };
    Backend _backend{ Backend::WINDOW };

    // Several physical keys can drive one logical key (W and the up arrow): it goes down with the first, up with
    // the last.
    std::array<uint8_t, static_cast<std::size_t>( LogicalKey::COUNT )> _sourcesDown{};
    struct Edge { LogicalKey key; bool down; };
    std::vector<Edge> _edges;  // this frame's presses and releases, in order

    static auto nativeToLogical( int code ) -> LogicalKey;  // evdev KEY_* on Linux, VK_* on Windows

#if defined(_WIN32)
    HANDLE _conin{ INVALID_HANDLE_VALUE };
    std::array<bool, 256> _vkDown{};  // held keys, from GetAsyncKeyState (CONSOLE and ASYNC_KEYS)
    std::vector<DWORD> _ancestors;     // our process and its parents, for the focus check
    HWND _typedWindow{};               // the foreground window when our console last got a key
    auto terminalHasFocus() -> bool;
    void pollConsole();
    void pollAsyncKeys();
#else
    std::vector<int> _evdevFds;
    void openEvdevKeyboards();
    void pollEvdev();
#endif
};
