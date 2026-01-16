#include <iostream>
#include <functional>
#include <bicycle/ActionRegistry.h>
#include <cxxabi.h>
#include <typeinfo>

using namespace std;
using enum ActionState;

char* __cxa_demangle(const char* __mangled_name, char* __output_buffer, size_t* __length, int* __status);

struct Man {
  int hp;
  int mp;
  double intuition;
};

ACT( 
  C_Action, 
  [&]() {
    auto b = get<double>("hi");
    return FAILED;
  }
);

// Now to figure out how to get references to talk directly through.
int main() {
  auto& reg = ActionRegistry::getInstance();
  auto cc = reg.get( "C_Action" );
  cc->set<Man>( "hi", { 45, 42, 85.0 });
  cc->f();
  return 0;
}
