#include <yaml-cpp/yaml.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

using namespace std;

template<typename T>
class A {
  protected:
    T t;
    int _i;
};

class B : protected A<int> {
  public:
    B(int i) {
      _i = i;
      cout << "i is " << _i << "\n";
    }
};

int main() {
  B b{ 45 };
  return 0;
}
