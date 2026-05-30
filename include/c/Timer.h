#pragma once
#include <array>
#include <string>
#include <bitset>
#include <chrono>

#include "Constants.h"

using Entity = unsigned;

constexpr unsigned long MAX_NUM_TIMERS{1024};

// TODO perhaps this is a good candidate for 
struct TimeoutMsg {
  Entity entity{};
  std::string val;  // TODO replace with a proper enum
};

using namespace std::chrono_literals;
// Interval allows us to know how much of the current frame is left for sleeping.
constexpr std::chrono::milliseconds INTERVAL(static_cast<int>(MILLISECONDS_PER_FRAME));

using TimerId = unsigned;

class Timer {
  public:
    static auto getInstance() -> Timer&;
    static void run();
    static void sleepFrame();

    // Timer-specific functions
    void _run();
    auto start( const unsigned timeMs, const Entity entity, const TimeoutMsg& timeoutMsg ) -> TimerId;  // returns the ID of the timer started for caller's future reference
    void stop( TimerId timerId );
    void pause( const TimerId timerId );
    void unpause( const TimerId timerId );
    void setDuration( const TimerId timerId, const unsigned durMs );
    
  private:
    Timer();
    Timer(const Timer&) = delete;
    Timer operator=(const Timer&) = delete;
    Timer(const Timer&&) = delete;
    Timer operator=(const Timer&&) = delete;
    std::bitset<MAX_NUM_TIMERS> _availableTimers{ ~std::bitset<MAX_NUM_TIMERS>{} }; // 1 = unused
    std::array<unsigned, MAX_NUM_TIMERS> _decrementers{};
    std::array<TimeoutMsg, MAX_NUM_TIMERS> _msgs{};
    auto findAvailableTimer() -> unsigned;
    std::array<unsigned, MAX_NUM_TIMERS> _times{};
    std::chrono::time_point<std::chrono::steady_clock> frameStartTime{};
};
