#include "c/Timer.h"
#include "Constants.h"
// TODO #include "c/Trigger.h"
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
  for ( unsigned i = 0; i < MAX_NUM_TIMERS; ++i ) {
    _times.at(i) -= _decrementers.at(i);
    if ( _decrementers.at(i) > 0 && _times.at(i) == 0 ) {
      // TODO activity.onTimer( _msgs.at(i) );
      auto& msg = _msgs.at(i);
      // Unconditionally reset the timer to avoid complex branching.
      _times.at(i) = msg.full;
      std::cout << "Timer " << msg.val << " fired.\n";
    }
  }
}

void Timer::pause( const unsigned timerId ) {
  _decrementers.at(timerId) = 0;
}

void Timer::unpause( const unsigned timerId ) {
  _decrementers.at(timerId) = 1;
}

auto Timer::start( const unsigned timeMs, const Entity entity, const std::string& timeoutMsg ) -> unsigned {
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
    _msgs.at(timerId)  = TimeoutMsg{ entity, timeoutMsg, nFrames };
  }
  return timerId;
}

void Timer::stop( unsigned timerId ) {
  _decrementers.at(timerId) = 0;
  _availableTimers.set( timerId, true );
}

void Timer::setDuration( unsigned timerId, unsigned durMs ) {
  _msgs.at( timerId ).full = (FRAMES_PER_SECOND * durMs) / 1000;
}

auto Timer::findAvailableTimer() -> unsigned {
  const auto idx = _availableTimers._Find_first();  // _Find_first() is a g++ extension. Lucky me.
  _availableTimers.set( idx, false );
  return idx;
}
