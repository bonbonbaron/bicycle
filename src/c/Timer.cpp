#include "c/Timer.h"
#include "Constants.h"
#include "c/Trigger.h"

#include <iostream> // TODO delete
#include <thread>


Timer::Timer() : frameStartTime( std::chrono::steady_clock::now() ) {}

auto Timer::getInstance() -> Timer& {
  static Timer timer;
  return timer;
}

void Timer::run() {
  auto& t = Timer::getInstance();
  t._run();
}

// Sleeps for the rest of this frame
void Timer::sleepFrame() {
  auto& t = Timer::getInstance();
  // Sleep till the end of this frame.
  auto elapsed = std::chrono::steady_clock::now() - t.frameStartTime;
  if (elapsed < INTERVAL) {
    std::this_thread::sleep_for(INTERVAL - elapsed);
  }
  // Mark the start of the next frame.
  t.frameStartTime = std::chrono::steady_clock::now();
}

void Timer::_run() {
  for ( unsigned timerId = 0; timerId < MAX_NUM_TIMERS; ++timerId ) {
    _times.at(timerId) -= _decrementers.at(timerId);
    if ( _decrementers.at(timerId) > 0 && _times.at(timerId) == 0 ) {
      Trigger::onTimer( _msgs.at(timerId) );
    }
  }
}

void Timer::pause( const unsigned timerId ) {
  _decrementers.at(timerId) = 0;
}

void Timer::unpause( const unsigned timerId ) {
  _decrementers.at(timerId) = 1;
}

auto Timer::start( const unsigned timeMs, const unsigned timerType, const std::string& timeoutMsg ) -> unsigned {
  auto timerId = findAvailableTimer();
  if ( timerId < MAX_NUM_TIMERS )  {
    // float mult faster than div
    //TODO: figure out which performs better on each  machine
#if 0
    unsigned nFrames{static_cast<unsigned>(timeMs * FRAMES_PER_MILLISECOND)};
#else
    // frames/sec * sec/msec * msec = frames
    unsigned nFrames{ (FRAMES_PER_SECOND * timeMs) / 1000 };
#endif
    _times.at( timerId ) = nFrames;
    _decrementers.at(timerId) = 1;
    _msgs.at(timerId)  = TimeoutMsg{ timerId, timerType, timeoutMsg };
  }
  return timerId;
}

void Timer::stop( unsigned timerId ) {
  _decrementers.at(timerId) = 0;
  _availableTimers.set( timerId, true );
}

auto Timer::findAvailableTimer() -> unsigned {
  const auto idx = _availableTimers._Find_first();  // _Find_first() is a g++ extension. Lucky me.
  _availableTimers.set( idx, false );
  return idx;
}
