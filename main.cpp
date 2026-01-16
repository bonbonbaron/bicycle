#include <iostream>
#include <functional>
#include <bicycle/ActionRegistry.h>

using namespace std;
using enum ActionState;

ACT( 
  C_Action, 
  [&]() {
    auto b = get<int>("hi");
    cout << "seeing int " << b << "\n";
    return FAILED;
  }
);

int main() {
  auto& reg = ActionRegistry::getInstance();
  auto cc = reg.get( "C_Action" );
  cc->set<int>( "hi", 45 );
  cc->f();
  return 0;
}
