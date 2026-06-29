#include "bicycle.h"
#include <iostream>
#include "v/TextMenu.h" // TODO remov when done testing
#include <string>
#include <vector>

void doNothing() {}
void pushMenu() {
  std::vector<Menu::MenuItem> v{
    { { "itemA", 0 }, "Aslan", doNothing },
    { { "itemB", 1 }, "Barton", doNothing },
    { { "itemC", 2 }, "Catie", doNothing },
    { { "itemD", 3 }, "Dumbass", doNothing }
  };
  bicycle::push<TextMenu>( "some text menu", v, 15, 15, 10, 10 );
}


int main( int argc, char** argv ) {
  if ( argc != 2 ) {
    std::cerr << "Expected use:\n\t./o <game-name>\nIf you're including the directory or extension of your game's name, don't.\n";
    exit(1);
  }
  bicycle::init( argv[1] );
  //==========================
  // ___ start test here ___
  //==========================
  std::vector<Menu::MenuItem> v{
    { { "itemA", 0 }, "Aslan", doNothing },
    { { "itemB", 1 }, "Barton", doNothing },
    { { "itemC", 2 }, "Catie", doNothing },
    { { "itemD", 3 }, "Dumbass", pushMenu }
  };
  bicycle::push<TextMenu>( "some text menu", v, 10, 10, 10, 10 );
  //==========================
  // ___ finish test here ___
  //==========================
  return bicycle::run();
}
