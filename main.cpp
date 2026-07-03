#include <iostream>
#include <list>
#include <vector>

using namespace std;


int main( int argc, char** argv) {
  list<int> a{ 1, 2, 3};
  list<int> b{ 5, 8, 4};
  a.merge(b);

  for ( auto& aa : a ) cout << aa << '\n';
  for ( auto& bb : b ) cout << bb << '\n';
  return 0;
}
