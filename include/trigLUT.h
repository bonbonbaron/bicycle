#include <array>

constexpr unsigned LUT_RSHIFT{16};
extern std::array<int, 256> sineLUT;
extern std::array<std::array<int, 256>, 256> atan2LUT;
