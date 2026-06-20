#include "bicycle.h"
#include <iostream>
#include "v/Dialogue.h" // TODO remov when done testing

int main( int argc, char** argv ) {
  if ( argc != 2 ) {
    std::cerr << "Expected use:\n\t./o <game-name>\nIf you're including the directory or extension of your game's name, don't.\n";
    exit(1);
  }
  bicycle::init( argv[1] );
  // ___ start test here ___
  bicycle::push<Dialogue>( "hello world", 5, 5 );
  std::cout << "\e[92mThou mayest beginnnn!\e[0m\n";
  return bicycle::run();
}
