#include <cmath>
#include <iostream>
#include <array>

int main() {
  constexpr unsigned RSHIFT{16};
  std::cout << "#include <array>\n\n";
  std::cout << "constexpr unsigned LUT_RSHIFT{" << RSHIFT << "};\n";
  std::cout << "std::array<int, 256> sineLUT{ " << "\n";
  for ( int i = 0; i < 256; ++i ) {
    auto j = static_cast<int>( sin( i * M_PI/128 ) * (1<<RSHIFT) );
    std::cout << "\t" << j << ",\t// " << i << "\n";
  }
  std::cout << "};  // sine LUT" << "\n";

  std::cout << "std::array<std::array<int, 256>, 256> atan2LUT{ {" << "\n";
  for ( int y = 0; y < 256; ++y ) {
    std::cout << "\t{";
    for ( int x = 0; x < 256; ++x ) {
      auto result = static_cast<int>( atan2( y, x ) * (1<<16) );
      std::cout << result << ", ";
    }
    std::cout << "},\n";
  }
  std::cout << "} }; // atan2 LUT" << "\n";

  return 0;
}
