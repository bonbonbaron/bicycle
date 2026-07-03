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
#include "m/World.h"

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
    auto& rect = World::get<Rect>( entity );
    rect.pos += { 1, 1 };
  }
}

T(t1) {
  // auto& pos = World::get<Position>( entity );
  // This does nothing for now.
  // auto& timer = Timer::getInstance();
  // pos -= { 1, 1 };
  // timer.start( 333, entity, 0, "do it again" );
}

T(t2) {
  auto& rect = World::get<Rect>( entity );
  rect.pos += { 1, 1 };
  auto& timer = Timer::getInstance();
  timer.start( 500, entity, 0, "do it again" );
}

C(c1) {
  std::cout << entity << " says YEEEEEEOUCH!!!\n";
}

/* Goals;
 * 
 * \get input to entity through Trigger
 * \get timer to entity through Trigger
 * get collision to entity through Trigger
 */

Battle::Battle() {
  auto& trig = Trigger::getInstance();
  auto& timer = Timer::getInstance();

  // Set up the grid
  Layer l1{ "abk\nabk\nabc\nabc\nabb\nabb" };
  Layer l2{ "idkyet\nidkyet\nidc\nidc\nidbyet\nidb" };
  Grid g{};
  g.addLayer( l1 );
  g.addLayer( l2 );
  Entity e = newEntityId();
  Entity o = newEntityId();
  g.addEntity( e, 1 );
  g.addEntity( o, 1 );
  std::cout  << "e is " << e << ", o is " << o << "\n";

  // make personality
  Personality ep;
  ep.setQuirk<InputState>( i1 );
  ep.setQuirk<Timeout>( t1 );
  ep.setQuirk<Collision>( c1 );

  Personality op;
  op.setQuirk<Collision>( c1 );
  op.setQuirk<Timeout>( t2 );
  
  // Set globally accessible traits
  World::set<Image>( e, { "r", Color::RED } );
  World::set<Image>( o, { "b", Color::BLUE } );
  World::set<Rect>( e, {{ 12, 12, 0}, {1, 1}} );
  World::set<Rect>( o, {{14, 14, 0}, { 1, 1}});
  World::set<CollRect>( e, {{{ 0, 0 }, {1, 1}}, 123} );
  World::set<CollRect>( o, {{{0, 0}, { 1, 1}}, 456} );
  trig.setPersonality( e, ep );
  trig.setPersonality( o, op );
  CollisionDetector::registerEntity( e, 1 );
  CollisionDetector::registerEntity( o, 1 );

  // Push the scene window up
  bicycle::push<Scene>( g );
  timer.start( 1000, e, 0, "whatever" );  // start a timer on entity just for fun
  timer.start( 500, o, 0, "whatever" );  // start a timer on entity just for fun

  // I see no reason why bicycle::<whatever> should have a function for the above.

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
