#include <iostream>
#include <bitset>

using namespace std;


int main() {
  bitset<5> i{3};
  bitset<5> j{4};
  auto k = i | j;
  cout << "i: " << i << '\n';
  cout << "j: " << j << '\n';
  cout << "k: " << k << '\n';
  return 0;
}
