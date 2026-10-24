#pragma once
// The one keyboard listener for every mode and platform. Every key source calls press( key, down ) on an edge of
// a physical key; listen() (once per frame, from the renderer's pollInput) polls the terminal backend if there is
// one and hands the resulting presses and releases to Trigger::sendInput, in order.
//
//   2D / 3D (SDL window, any OS)      PixelRenderer's SDL event pump calls press(); nothing else to poll
//   ASCII, Linux local console / Pi   evdev keyboards (/dev/input/event*, needs the "input" group)
//   ASCII, Linux over SSH or no evdev the terminal (SshInput: raw stdin, kitty key releases or auto key-up)
//   ASCII, Windows with a console     CONIN$ key records (conhost, Windows Terminal, VS Code, mintty with ConPTY)
//   ASCII, Windows without one        GetAsyncKeyState (mintty with a piped stdin)
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
    std::array<bool, 256> _vkDown{};
    void pollConsole();
    void pollAsyncKeys();
#else
    std::vector<int> _evdevFds;
    void openEvdevKeyboards();
    void pollEvdev();
#endif
};
