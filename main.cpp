#include <yaml-cpp/yaml.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <any>
#include <functional>
#include <bicycle/ActionRegistry.h>

using namespace std;
using enum ActionState;

struct C : public Action {
  C() : Action( "C Action", 
        [&]() {
          auto bb = getBlackboard();
          auto b = bb->get<int>("hi");
          cout << "seeing int " << b << "\n";
          return FAILED;
        }) {}
};

static C c;

int main() {
  auto& reg = ActionRegistry::getInstance();
  auto cc = reg.get( "C Action" );
  auto bb = cc->getBlackboard();
  bb->set<int>( "hi", 45 );
  cc->f();
  return 0;
}
