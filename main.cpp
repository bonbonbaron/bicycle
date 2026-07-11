#include <iostream>
#include <string>
#include <algorithm> // Required for std::remove

struct A : public int {
  int b;
};

int main() {
    std::string str = "Hello, World!";
    A a{};

    std::cout << a << std::endl; // Outputs: Heo, Word!
    return 0;
}

