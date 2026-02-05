#include "bicycle.h"
#include <iostream>

int main( int argc, char** argv ) {
  if ( argc != 2 ) {
    std::cerr << "Expected use\n\t./o <game-name>\n";
    exit(1);
  }
  std::string gameName = argv[1];
  bicycle::init( gameName );
  return bicycle::run();
}
