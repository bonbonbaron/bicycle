#include "bicycle.h"
#include <iostream>
#include "v/TextMenu.h" // TODO remov when done testing
#include <string>
#include <vector>
#include "v/Dialogue.h"
#include "v/Scene.h"

/*
 * TODO
 *  1) first try to do Battle in C++
 *  2) then see what it's like to do it in Lua
 */

struct Character {
  Character( const std::string&& name ) : name(name) {}
  std::string name;
};

class Battle {
  public:
    Battle() = default;
    void init();
};

void Battle::init() {
  Layer l{ "idkidkidcidcidbidb" };
  Grid g{};
  g.addLayer( l );

  bicycle::push<Scene>( g );
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
  Battle b;
  b.init();
  //==========================
  // ___ finish test here ___
  //==========================
  return bicycle::run();
}
