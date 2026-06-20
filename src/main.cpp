#include "bicycle.h"
#include <iostream>
#include "v/Dialogue.h" // TODO remov when done testing

int main( int argc, char** argv ) {
  //std::cout << MASK_A << '\n';
  //std::cout << MASK_B << '\n';
  //return 0;
  if ( argc != 2 ) {
    std::cerr << "Expected use:\n\t./o <game-name>\nIf you're including the directory or extension of your game's name, don't.\n";
    exit(1);
  }
  bicycle::init( argv[1] );
  // ___ start test here ___
  bicycle::push<Dialogue>( "see i don't want to go to the store and buy a bunch of apples if it means i have to give up my virginity to the lady at the cash register who weighs 500 pounds okay?", 5, 5, 20, 20 );
  std::cout << "\e[92mThou mayest beginnnn!\e[0m\n";
  return bicycle::run();
}
