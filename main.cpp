#include <iostream>
#include <string>
#include <memory>

template <typename T>
void f( T& t ) {
  std::cout << "t is " << t << "\n";
}

template <typename T>
void g( T& t ) {
  f<decltype(t)>(t);
}

template <typename T>
void g( std::shared_ptr<T>& t ) {
  f<decltype(*t)>(*t);
}

int main() {
  int i = 45;

  auto j = std::make_shared<decltype(i)>( i );

  return 0;
}
