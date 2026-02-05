#include <iostream>
#include <bicycle/ConditionRegistry.h>
#include <bicycle/Personality.h>
#include <bicycle/Dialogue.h>
#include <bicycle/bicycle.h>
#include <bicycle/Body.h>
#include <bicycle/Dice.h>
#include <bicycle/Timer.h>
#include <chrono>
#include <thread>

#include <cursesw.h>

#include "Config.h"

using enum ActionState;

struct Stats {
  int age;
  int num;
  std::string buddy;
};

F_( plan,
  Dice dirDice{ 0, 3 };
  Position delta{};
  switch ( dirDice.roll() ) {
    case 0:
      delta.x = 1;
      break;
    case 1:
      delta.x = -1;
      break;
    case 2:
      delta.y = 1;
      break;
    default:
      delta.y = -1;
      break;
  }
  arg.set<Position>( "vel", delta );

  Dice sleepDice{ 100, 1000 };
  std::this_thread::sleep_for( std::chrono::milliseconds( sleepDice.roll() ) );

  return IN_PROGRESS;
);

// F_( mv,
ActionState mv( ActArg& arg ) {
  auto& pos = arg.get<std::shared_ptr<Position>>( "pos" );
  auto& vel = arg.get<Position>( "vel" );
  *pos += vel;
  auto& wm = WindowManager::getInstance();
  mvprintw( COLS - 1, 1, "moved to (%d, %d)", pos->y, pos->x );
  wm.render();
  return IN_PROGRESS;
}

F_( sayHi, std::cout << "tree1: Hi there!\n"; return COMPLETE; );
F_( sayHo, std::cout << "tree2: Ho there!\n"; return FAILED; );
F_( sayHe, std::cout << "tree2: He there!\n"; return COMPLETE; );
F_( chooseTgt, 
    std::cout << "both trees: Thinking...\n"; 
    std::cout << " both trees: Choosing my target\n"; 
    return FAILED; );
F_( eatTgt, std::cout << "both trees: Eating my target\n"; return COMPLETE; );
F_( onInput,
    auto& input = arg.get<char>("input");
    auto& posPtr = arg.get<std::shared_ptr<Position>>("pos");
    switch( input ) {
      case 'h':
      case 'H':
        --posPtr->x;
      break;
      case 'j':
      case 'J':
        ++posPtr->y;
      break;
      case 'k':
      case 'K':
        --posPtr->y;
      break;
      case 'l':
      case 'L':
        ++posPtr->x;
      break;
    }
    return COMPLETE;
  );

  // how can i call these register functions in the core library while still picking up the configs for each game?
  // i can just store them in a structre for the game instead of registering straight up
  // and then i can iterate through a vector of that structure
  //

static void registerPortTypes() {
  PORT( age, int );
  PORT( hi, Stats );
  PORT( hello, int );
  PORT( there, int );
  PORT( time, int );
  PORT( name, std::string );
  PORT( input, char );
  PORT( pos, std::shared_ptr<Position> );
  PORT( vel, Position );
}

// TODO automate this, perhaps replacing F_ macro with template
static void registerActions() {
  ACT( sayHi, "hello", "there" );
  ACT( sayHo );
  ACT( sayHe );
  ACT( chooseTgt );
  ACT( eatTgt );
  ACT( onInput, "pos", "input" );
  ACT( plan, "vel" );
  ACT( mv, "pos", "vel" );
}

auto imnotgay ()-> bool {
  std::cout << "I SWEAR TO GAWD\n"; 
  return true; 
}

// TODO automate this, perhaps replacing F_ macro with template
static void registerConditions() {
  COND( imnotgay );
}

static void registerBlackboards() {
  BB ( bb1, 
      { "age", 42 },
      { "hi", Stats{ 59, 123, "Charlie" } },
      { "hello", 8012 },
      { "there", 800 },
      { "time", 0 },
      { "name", std::string{"Michael"} },
      { "input", ' ' },
      { "pos", std::shared_ptr<Position>() },
      { "vel", Position{} }
     );
}

void config() {
  registerPortTypes();
  registerActions();
  registerConditions();
  registerBlackboards();
}
