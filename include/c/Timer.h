#pragma once
#include "c/System.h"

constexpr unsigned long MAX_NUM_TIMERS{1024};

class Timer : public System {
  public:
    Timer();

    // Overrides
    void run() override;
    void enable( const Entity entity ) override;  // turns 0 to 1 in time decrementer
    void disable( const Entity entity ) override;
    void pause( const Entity entity ) override;

    // Timer-specific functions
    
  private:
    std::bitset _timersUsed{MAX_NUM_TIMERS};
    std::array<unsigned, MAX_NUM_TIMERS> _decrementers;
};
