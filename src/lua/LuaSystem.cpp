// Lua FFI: the engine itself: assets (the game's pack or loose files, see data/Pack.h), the build, frame rate, dice.
#include "export.h"
#include "c/Dice.h"
#include "data/Mem.h"
#include "data/Pack.h"
#include <string>

extern "C" {

// The asset's bytes (valid until the next call) and their length, or null.
EXPORT_API const char* assetRead( const char* path, std::size_t* len ) {
  static thread_local mem::Blob buf;
  auto data = assets::read( path, mem::TEXT );
  if ( !data ) return nullptr;
  buf = std::move( *data );
  *len = buf.size();
  return buf.data();
}

EXPORT_API bool assetExists( const char* path ) { return assets::exists( path ); }
EXPORT_API bool assetPacked() { return assets::mounted(); }

// Newline-separated paths of the pack's files in a folder ("" without a pack).
EXPORT_API const char* assetList( const char* dir ) {
  static thread_local std::string buf;
  buf.clear();
  for ( const auto& p : assets::list( dir ) ) buf += p + "\n";
  return buf.c_str();
}

// Release builds treat the generated ids as complete (scripts/Ids.lua).
EXPORT_API bool isReleaseBuild() {
#ifdef BICYCLE_RELEASE_ASSETS
  return true;
#else
  return false;
#endif
}

EXPORT_API int rollDice( const int min, const int max ) { return Dice( min, max ).roll(); }

}  // extern "C"
