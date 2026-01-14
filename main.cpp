#include <yaml-cpp/yaml.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <any>

using namespace std;

using Map = map<int, any>;

struct A {
  double d{45};
  string s{"your mother"};
};

int main() {
  Map m;
  m[1] = 45;
  m[2] = "hello there";
  m[3] = A();

  cout << any_cast<A>(m[3]).s << "\n";
  return 0;
}
