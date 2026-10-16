#pragma once
// BICYCLE_STARTUP_STATS=1: one stderr line per start-up phase, with the time since main() and the phase's own
// time, to find what makes a cold start slow (tests read nothing from it; release builds print it too: stderr only).
//   startup: <phase>  +<ms since the previous mark>  @<ms since main>
// The time before main() (the dynamic loader mapping the executable and its DLLs/.so's, and on Windows any
// antivirus scan of them) isn't visible from in here: compare "@first frame" with the wall time of the whole run.
#include <chrono>
#include <cstdio>
#include <cstdlib>

namespace startup {
inline auto on() -> bool {
  static const bool v = [] { const char* e = std::getenv( "BICYCLE_STARTUP_STATS" ); return e && *e && *e != '0'; }();
  return v;
}
inline auto origin() -> std::chrono::steady_clock::time_point {
  static const auto t = std::chrono::steady_clock::now();
  return t;
}
inline void mark( const char* phase ) {
  using clk = std::chrono::steady_clock;
  static clk::time_point last = origin();
  if ( !on() ) return;
  const auto now = clk::now();
  auto ms = []( clk::duration d ) { return std::chrono::duration<double, std::milli>( d ).count(); };
  std::fprintf( stderr, "startup: %-34s +%8.1f ms  @%8.1f ms\n", phase, ms( now - last ), ms( now - origin() ) );
  last = now;
}
}  // namespace startup
