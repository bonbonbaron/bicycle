#include <iostream>
#include <bicycle/Personality.h>

using namespace std;
using enum ActionState;

ACT( 
  A, 
  [&]() {
    auto a = get<double>("a");
    return ( a < 11.0 ) ? SUCCESS : FAILED;
  }
);

ACT( 
  B, 
  [&]() {
    auto b = get<double>("b");
    return ( b > 12.0 ) ? SUCCESS : FAILED;
  }
);

ACT( 
  C, 
  [&]() {
    auto c = get<double>("c");
    return ( c > 13.0 ) ? SUCCESS : FAILED;
  }
);

ACT( 
  D, 
  [&]() {
    auto d = get<double>("d");
    return ( d > 14.0 ) ? SUCCESS : FAILED;
  }
);

// Now to figure out how to get references to talk directly through.
int main() {
  auto& reg = ActionRegistry::getInstance();
  auto cc = reg.get( "C" );
  cc->set<double>( "hi", 42.0 );
  cc->f();
  return 0;
}
