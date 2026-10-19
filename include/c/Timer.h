#pragma once
#include <array>
#include <string>
#include <bitset>
#include <chrono>

#include "Constants.h"
#include "c/Controllable.h"
#include "c/Timeout.h"
#include "data/ActiveFirst.h"
#include <vector>

constexpr unsigned long MAX_NUM_TIMERS{1024};

using namespace std::chrono_literals;
// Interval allows us to know how much of the current frame is left for sleeping.
constexpr std::chrono::milliseconds INTERVAL(static_cast<int>(Constants::MILLISECONDS_PER_FRAME));

using TimerId = unsigned;

class Timer : public Controllable {
  public:
    static auto getInstance() -> Timer&;
    static void run();
    static void sleepFrame();

    // Timer-specific functions
    void _run();
    auto create( const unsigned timeMs, Entity entity, const unsigned timerType, const bool repeat, const TimeoutAddr addr = TimeoutAddr::BRIDGE) -> TimerId;  // returns the ID of the timer started for caller's future reference
    void start( TimerId timerId) override;  // returns the ID of the timer started for caller's future reference
    void stop( TimerId timerId ) override;
    void pause( const TimerId timerId ) override;
    void unpause( const TimerId timerId ) override;

    void setDuration( const TimerId timerId, const unsigned durMs );
    void stopEntity( Entity entity );  // every timer whose timeouts go to entity (a despawned entity)
    
  private:
    Timer();
    Timer(const Timer&) = delete;
    Timer operator=(const Timer&) = delete;
    Timer(const Timer&&) = delete;
    Timer operator=(const Timer&&) = delete;
    auto findAvailableTimer() -> unsigned;
    std::bitset<MAX_NUM_TIMERS> _availableTimers{ ~std::bitset<MAX_NUM_TIMERS>{} }; // 1 = unused
    // The timers in use, keyed by id; the running ones are the active rows, so a frame counts down only those.
    // Frames left (0: due), the full period, whether it repeats, and what it says when it times out.
    enum { LEFT, FULL, REPEAT, MSG };
    ActiveFirst<unsigned, unsigned, uint8_t, Timeout> _timers;
    std::vector<TimerId> _due;  // _run(): this frame's
    std::chrono::time_point<std::chrono::steady_clock> frameStartTime{};
};
