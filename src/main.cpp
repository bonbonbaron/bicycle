#include "bicycle.h"
#include <iostream>
#include "v/TextMenu.h" // TODO remov when done testing
#include <string>
#include <vector>
#include "v/Dialogue.h"

struct Character {
  Character( const std::string&& name ) : name(name) {}
  std::string name;
};


class Battle {
  public:
    Battle() = default;
    void init();
  private:
    void pushMenu();
    void doNothing();
    std::vector<Character> chars;
    int charnum{};
};

static int j{0};
void Battle::pushMenu() {
  
  std::vector<Menu::MenuItem> v {
    { { "itemA", 0 }, "yell", [&]{ 
                                   charnum = ( charnum + 1 ) % chars.size();
                                   auto c = chars.at( charnum ); 
                                   bicycle::push<Dialogue>( "YARGGGHH!!! MY NAME IS " + c.name ); } 
    },
    //{ { "itemB", 1 }, "whisper", [&]{ std::cout << "pssst! my name is " << c.name << '\n'; bicycle::pop(); } },
  };
  bicycle::push<TextMenu>( "some text menu", v, 5 + j*2, 5 + j*2, 10, 15 );
  ++j;
}

void Battle::init() {
  chars.emplace_back( "Bohemond" );
  chars.emplace_back( "Redmond" );
  chars.emplace_back( "Teddymond" );
  pushMenu();
}
  
void Battle::doNothing() {}


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
