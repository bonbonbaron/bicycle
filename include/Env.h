#pragma once
// Environment switches (BICYCLE_*), read the same way everywhere.
#include <cstdlib>

namespace env {
// Set, not empty and not "0": BICYCLE_SCENE_STATS=1 and the like.
inline auto flag( const char* name ) -> bool {
  const char* v = std::getenv( name );
  return v && *v && *v != '0';
}
}  // namespace env
