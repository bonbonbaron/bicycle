#include <cmath>
#include <iostream>
#include <array>
#include <unordered_set>
using namespace std;

struct A {
  int i;
  int j;
};
int main() {
  unordered_set<A> u{};
  u.insert( A{1, 2} );
  return 0;
}
