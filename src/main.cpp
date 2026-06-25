#include "bicycle.h"
#include <iostream>
#include "v/TextMenu.h" // TODO remov when done testing

int main( int argc, char** argv ) {
  if ( argc != 2 ) {
    std::cerr << "Expected use:\n\t./o <game-name>\nIf you're including the directory or extension of your game's name, don't.\n";
    exit(1);
  }
  bicycle::init( argv[1] );
  //==========================
  // ___ start test here ___
  //==========================
  bicycle::push<TextMenu>( 
      "see i don't want to go to the store and buy a bunch of apples if it means i have to give up my virginity to the lady at the cash register who weighs 500 pounds okay?",
      5,
      5,
      20,
      20 
  );
  //==========================
  // ___ finish test here ___
  //==========================
  return bicycle::run();
}
