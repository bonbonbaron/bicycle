#include "bicycle.h"
#include <iostream>

int main( int argc, char** argv ) {
  if ( argc != 2 ) {
    std::cerr << "Expected use:\n\t./o <game-name>\nIf you're including the directory or extension of your game's name, don't.";
    exit(1);
  }
  return bicycle::run( argv[1] );
}
