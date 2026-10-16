#pragma once
// Headless captures for docs and tests (dev builds only; BICYCLE_RELEASE writes no files):
//   BICYCLE_SCREENSHOT=<path>          the file to save; a "%d" in it is replaced by the frame number
//   BICYCLE_SCREENSHOT_FRAMES=40,50,60 which frames to save (else BICYCLE_SCREENSHOT_FRAME, default 90)
// Pixel modes save .bmp images; ASCII mode saves the screen's text.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>

namespace capture {

inline auto pathFor( long frame ) -> std::optional<std::string> {
#ifdef BICYCLE_RELEASE
  (void)frame;
  return std::nullopt;
#else
  const char* path = std::getenv( "BICYCLE_SCREENSHOT" );
  if ( !path || !*path ) return std::nullopt;
  bool due = false;
  if ( const char* list = std::getenv( "BICYCLE_SCREENSHOT_FRAMES" ); list && *list ) {
    for ( const char* p = list; *p; ) {
      char* end{};
      const long f = std::strtol( p, &end, 10 );
      if ( end == p ) break;
      if ( f == frame ) due = true;
      p = *end ? end + 1 : end;
    }
  }
  else {
    const char* one = std::getenv( "BICYCLE_SCREENSHOT_FRAME" );
    due = frame == ( one ? std::atol( one ) : 90 );
  }
  if ( !due ) return std::nullopt;
  std::string out = path;
  if ( auto at = out.find( "%d" ); at != std::string::npos ) out.replace( at, 2, std::to_string( frame ) );
  return out;
#endif
}

// Whether the last listed frame has been saved (BICYCLE_EXIT_AFTER_SCREENSHOT=1 then quits).
inline auto lastFrame() -> long {
  long last = -1;
  if ( const char* list = std::getenv( "BICYCLE_SCREENSHOT_FRAMES" ); list && *list ) {
    for ( const char* p = list; *p; ) {
      char* end{};
      const long f = std::strtol( p, &end, 10 );
      if ( end == p ) break;
      last = f > last ? f : last;
      p = *end ? end + 1 : end;
    }
    return last;
  }
  const char* one = std::getenv( "BICYCLE_SCREENSHOT_FRAME" );
  return one ? std::atol( one ) : 90;
}

}  // namespace capture
