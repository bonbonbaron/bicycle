#include "bicycle.h"
#include <iostream>

int main( int argc, char** argv ) {
  if ( argc != 2 ) {
    std::cerr << "Expected use:\n\t./o <game-name>\nIf you're including the directory or extension of your game's name, don't.\n";
    exit(1);
  }
  std::cout << "\e[92mThou mayest beginnnn!\e[0m\n";
  return bicycle::run( argv[1] );
}
