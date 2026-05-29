#include <iostream>
#include <functional>
#include <string>

using namespace std;

template<typename T>
using f = function<int(int, T)>;

int a( int i, string j ){
  cout << "a() got string " << j << '\n';
  return i * 12;
}

template<typename T>
struct funcdata {
  f<T> func;
  int i{};
};


int main() {
  funcdata<string> data{ a, 2 };
  data.func( 12, "i am a string" );
  return 0;
}
