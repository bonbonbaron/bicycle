#pragma once
// The main loop's clock: the frame rate (fps() for Lua's fps()) and, with BICYCLE_FRAME_STATS=1, where a frame's
// time goes (ms, averaged over 90 frames, on stderr): motion + physics, Lua (events), rendering (with the GL work
// and the swap).
#include <array>
#include <chrono>

class FrameClock {
  public:
    enum Phase { PHYSICS, LUA, RENDER, N_PHASES };
    static auto get() -> FrameClock&;
    void beginFrame();        // after input, before the frame's work
    void endPhase( Phase p );  // in order: PHYSICS, LUA, RENDER
    auto fps() const -> float { return _fps; }

  private:
    using clk = std::chrono::steady_clock;
    FrameClock();
    clk::time_point _fpsFrom, _phaseFrom;
    int _fpsFrames{};
    float _fps{};
    bool _stats{};
    long _frames{};
    std::array<double, N_PHASES> _ms{};
};
