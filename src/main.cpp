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

/*
 * TODO
 *  1) first try to do Battle in C++
 *  2) then see what it's like to do it in Lua
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
struct Character {
  Character( const std::string&& name ) : name(name) {}
  std::string name;
};

class Battle {
  public:
    Battle();
};

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
  
  // Set globally accessible traits
  World::set<Image>( e, { "r", Color::RED } );
  World::set<Image>( o, { "b", Color::BLUE } );
  World::set<Rect>( e, {{ 16, 12, 0}, {1, 1}} );
  World::set<Rect>( o, {{14, 14, 0}, { 1, 1}});
  World::set<CollRect>( e, {{{ 0, 0 }, {1, 1}}, 123} );
  World::set<CollRect>( o, {{{0, 0}, { 1, 1}}, 456} );
  CollisionDetector::registerEntity( e, 1 );
  CollisionDetector::registerEntity( o, 1 );

  // Push the scene window up
  // bicycle::push<Scene>( g );
  auto tid1 = timer.create( 1000, e, 0, "whatever" );  // start a timer on entity just for fun
  auto tid2 = timer.create( 500, o, 0, "whatever" );  // start a timer on entity just for fun

  // I see no reason why bicycle::<whatever> should have a function for the above.

  // Ugh, i'm gonna have to figure this out later.
  // auto& wm = WindowManager::getInstance();
  // auto scene = std::static_pointer_cast<Scene>( wm.back() );
  // scene->setFocus( e );
}

int main( int argc, char** argv ) {
#if 1
  if ( argc != 2 ) {
    std::cerr << "Expected use:\n\t./o <game-name>\nIf you're including the directory or extension of your game's name, don't.\n";
    exit(1);
  }
  Battle b;
  bicycle::init( argv[1] );
  return bicycle::run();
#else
  std::cout << "Rect has " << TypeInfo<Rect>::field_count << " fields.\n";
	return 0;
#endif
}
#pragma GCC diagnostic pop
