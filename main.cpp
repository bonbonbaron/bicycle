#include <iostream>
#include "m/World.h"

using namespace std;

void f() {
  auto& times = World::get<Time>();
  times[253] = 111;
  times[247] = 111;
  times[251] = 111;
  times[250] = 111;
}


int main() {
  f();
  auto& t = World::get<Time>( 255 );
  t = 12345678;
  const auto& times = World::get<Time>();
  for ( const auto& t : times ) {
    std::cout << "time is " << t << '\n';
  }
  return 0;
}
