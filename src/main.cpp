#include "bicycle.h"
#include <iostream>
#include "v/TextMenu.h" // TODO remov when done testing
#include <string>
#include <vector>
#include "v/Dialogue.h"
#include "v/Scene.h"
#include <lua.hpp>
#include "v/ColorPalette.h"
#include "c/WindowManager.h"
#include "c/Trigger.h"
#include "m/World.h"
#include "TypeInfo.h"

int main( int argc, char** argv ) {
  if ( argc != 2 ) {
    std::cerr << "Expected use:\n\t./o <game-name>\nIf you're including the directory or extension of your game's name, don't.\n";
    exit(1);
  }
  bicycle::init( argv[1] );
  return bicycle::run();
}
