#include <iostream>
#include <vector>

using namespace std;


int main( int argc, char** argv) {
  vector<int> i{};
  for ( int j = 0; j < stoi(argv[1]); ++j ) {
    i.push_back(j);
    cout << (size_t)( &i ) << " with size " << i.size() << '\n';
  }
  return 0;
}
