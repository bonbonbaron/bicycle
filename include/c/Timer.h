#pragma once
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

class Timer {
  public:
    static auto getInstance() -> Timer&;
    static void run();

    // Timer-specific functions
    void _run();
    auto start( const unsigned timeMs, const Entity entity, const std::string& timeoutMsg, const int nReps = 0 ) -> unsigned;
    void stop( unsigned timerId );
    void pause( const unsigned timerId );
    void unpause( const unsigned timerId );
    void setDuration( const unsigned timerId, const unsigned durMs );
    
  private:
    Timer() = default;
    Timer(const Timer&) = delete;
    Timer operator=(const Timer&) = delete;
    Timer(const Timer&&) = delete;
    Timer operator=(const Timer&&) = delete;
    std::bitset<MAX_NUM_TIMERS> _availableTimers{ ~std::bitset<MAX_NUM_TIMERS>{} }; // 1 = unused
    std::array<unsigned, MAX_NUM_TIMERS> _decrementers{};
    std::array<TimeoutMsg, MAX_NUM_TIMERS> _msgs{};
    auto findAvailableTimer() -> unsigned;
    std::array<unsigned, MAX_NUM_TIMERS> _times{};
};
