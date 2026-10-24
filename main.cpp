#include <cmath>
#include <iostream>
#include <array>
#include <unordered_set>
using namespace std;

template<typename T>
void print( T arg ) {
	std::cout << arg;
}

template<typename T, typename ...Rest>
void printall(T t, Rest r...) {
	print(t);
	printall(r);
}

int main() {
	printall("i", " ", "love", " ", "you\n");
  return 0;
}
