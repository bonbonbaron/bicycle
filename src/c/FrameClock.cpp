#include "c/FrameClock.h"
#include "Env.h"
#include <cstdio>

auto FrameClock::get() -> FrameClock& {
  static FrameClock c;
  return c;
}

FrameClock::FrameClock() : _fpsFrom( clk::now() ), _stats( env::flag( "BICYCLE_FRAME_STATS" ) ) {}

void FrameClock::beginFrame() {
  const auto now = clk::now();
  _phaseFrom = now;
  ++_fpsFrames;  // fps: frames over the last half second or so
  if ( const double dt = std::chrono::duration<double>( now - _fpsFrom ).count(); dt >= 0.5 ) {
    _fps = static_cast<float>( _fpsFrames / dt );
    _fpsFrames = 0;
    _fpsFrom = now;
  }
}

void FrameClock::endPhase( Phase p ) {
  if ( !_stats ) return;
  const auto now = clk::now();
  _ms[p] += std::chrono::duration<double, std::milli>( now - _phaseFrom ).count();
  _phaseFrom = now;
  if ( p == RENDER && ++_frames % 90 == 0 ) {
    std::fprintf( stderr, "frame: physics %.2f ms, lua %.2f ms, render %.2f ms (avg of 90)\n", _ms[PHYSICS] / 90,
                  _ms[LUA] / 90, _ms[RENDER] / 90 );
    _ms = {};
  }
}
