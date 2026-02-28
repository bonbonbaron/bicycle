#pragma once
#include <random>

class Dice {
  public:
    Dice( const int min, const int max );
    Dice() = delete;
    auto roll() -> int;
  private:
    std::random_device _r;
    std::default_random_engine _e1;
    std::uniform_int_distribution<int> _uniform_dist;
};
