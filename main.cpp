#include <iostream>
#include <string>
#include <string_view>
#include <array>
#include <vector>

using namespace std;

int main() {
  vector<int> i;
  i.push_back(5);
  cout << i.back() << '\n';
  return 0;
}
