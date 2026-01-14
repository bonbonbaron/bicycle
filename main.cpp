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

struct B : public Action {
  B() : Action( "B action" ) {
    f = [&]() { 
      cout << "my favorite number is 45\n"; 
      return ActionState::SUCCESS;
    };
  }
};

struct C : public Action {
  C() : Action( "C Action" ) {
    f = [&]() {
      auto bb = getBlackboard();
      cout << "will this work?\n";
      return ActionState::FAILED;
    };
  }
};


static B b;
static C c;

int main() {
  Blackboard bb;
  bb.set<int>( "tiger", 45);
  cout << bb.get<int>("tiger") << "\n";

  return 0;
}
