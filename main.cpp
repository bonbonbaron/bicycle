#include <iostream>
#include <string>
#include <algorithm> // Required for std::remove

int main() {
    std::string str = "Hello, World!";
    char char_to_remove = 'l';

    // Erase-remove idiom
    str.erase(std::remove(str.begin(), str.end(), char_to_remove), str.end());

    std::cout << str << std::endl; // Outputs: Heo, Word!
    return 0;
}

