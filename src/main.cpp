#include "bicycle.h"
#include <iostream>
#include "v/TextMenu.h" // TODO remov when done testing
#include <string>
#include <vector>
#include "v/Dialogue.h"
#include "v/Scene.h"
#include <lua5.2/lua.hpp>
#include "v/ColorPalette.h"
#include "c/WindowManager.h"
#include "m/Personality.h"
#include "c/Trigger.h"

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
    Battle();
};

I(i1) {
  if ( input.currKeysPressed.any() ) {
    auto& pos = World::get<Position>( entity );
    pos += { 1, 1 };
  }
}

Battle::Battle() {
  Layer l1{ "abk\nabk\nabc\nabc\nabb\nabb" };
  Layer l2{ "idkyet\nidkyet\nidc\nidc\nidbyet\nidb" };
  Grid g{};
  g.addLayer( l1 );
  g.addLayer( l2 );
  Entity e = 5;
  g.addEntity( e, 1 );
  Personality p;
  p.setQuirk<const InputState>( i1 );
  auto& trig = Trigger::getInstance();
  trig.setPersonality( e, p );
  
  World::set<Position>( e, {12, 12} );
  World::set<Image>( e, { "r", Color::RED } );
  bicycle::push<Scene>( g );

  // Ugh, i'm gonna have to figure this out later.
  auto& wm = WindowManager::getInstance();
  auto scene = std::static_pointer_cast<Scene>( wm.back() );
  scene->setFocus( e );
}

/*  // FROM GROK:
 
   lua_State* L = luaL_newstate();  // Create Lua state
   if (!L) {
     std::cerr << "Failed to create Lua state\n";
     return 1;
   }

   luaL_openlibs(L);  // Load standard libraries (math, string, etc.)

   // Load and run a Lua script from file
   if (luaL_dofile(L, "script.lua") != LUA_OK) {
     std::cerr << "Error: " << lua_tostring(L, -1) << '\n';
     lua_pop(L, 1);  // Pop error message
   }

   lua_close(L);  // Clean up
*/

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
  //==========================
  // ___ finish test here ___
  //==========================
  return bicycle::run();
}
