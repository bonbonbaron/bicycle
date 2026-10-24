#include "bicycle.h"
#include <iostream>

#if defined( BICYCLE_RELEASE ) && defined( __linux__ )
#include <sys/prctl.h>
#endif

int main( int argc, char** argv ) {
#ifdef BICYCLE_RELEASE
#ifdef __linux__
  // A marker for tests/release_io_test.sh: everything strace sees after this prctl is the engine's doing (the
  // dynamic loader's opens come before it). Read-only and harmless.
  char name[16];
  prctl( PR_GET_NAME, name, 0, 0, 0 );
#endif
  // The game is linked in: `./o` runs it (an argument, if given, must name it).
  if ( argc > 2 || ( argc == 2 && std::string( argv[1] ) != BICYCLE_RELEASE_GAME ) ) {
    std::cerr << "This release build runs only its embedded game: ./o [" BICYCLE_RELEASE_GAME "]\n";
    exit(1);
  }
  bicycle::init( BICYCLE_RELEASE_GAME );
  return bicycle::run();
#endif
  if ( argc != 2 ) {
    std::cerr << "Expected use:\n\t./o <game-name>\nIf you're including the directory or extension of your game's name, don't.\n";
    exit(1);
  }
  // Tab completion adds a trailing slash ("plat3d/") and some shells a leading "./"; the engine joins paths
  // as "./<game>/...", so strip both rather than load from "./plat3d//...".
  std::string game = argv[1];
  while ( game.size() > 1 && ( game.back() == '/' || game.back() == '\\' ) ) game.pop_back();
  while ( game.rfind( "./", 0 ) == 0 || game.rfind( ".\\", 0 ) == 0 ) game.erase( 0, 2 );
  bicycle::init( game );
  return bicycle::run();
}
