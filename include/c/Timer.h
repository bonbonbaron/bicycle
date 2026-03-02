#pragma once
#include "c/System.h"
#include <array>
#include <string>
#include <bitset>

using Entity = unsigned;

constexpr unsigned long MAX_NUM_TIMERS{1024};

struct TimeoutMsg {
  Entity entity{};
  std::string val;
  int nReps{};
  unsigned full{};
};

class Timer : public System {
  public:
    Timer() = default;

    // Overrides
    void run() override;
    // BEcause timer has so little in common with other systems, i'm thjinkng screw the system class.

    // Timer-specific functions
    auto start( const unsigned timeMs, const Entity entity, const std::string& timeoutMsg, const int nReps = 0 ) -> unsigned;
    void stop( unsigned timerId );
    void pause( const unsigned timerId );
    void unpause( const unsigned timerId );
    void setDuration( const unsigned timerId, const unsigned durMs );
    
  private:
    std::bitset<MAX_NUM_TIMERS> _availableTimers{ ~std::bitset<MAX_NUM_TIMERS>{} }; // 1 = unused
    std::array<unsigned, MAX_NUM_TIMERS> _decrementers{};
    std::array<TimeoutMsg, MAX_NUM_TIMERS> _msgs{};
    auto findAvailableTimer() -> unsigned;
    std::array<unsigned, MAX_NUM_TIMERS> _times{};
};
