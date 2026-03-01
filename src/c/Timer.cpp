#include "c/Timer.h"
#include "Constants.h"
// TODO #include "c/Activity.h"
#include <iostream> // TODO delete

void Timer::run() {
  for ( unsigned i = 0; i < MAX_NUM_TIMERS; ++i ) {
    _times.at(i) -= _decrementers.at(i);
    if ( _decrementers.at(i) > 0 && _times.at(i) == 0 ) {
      // TODO activity.onTimer( _msgs.at(i) );
      auto& msg = _msgs.at(i);
      // Unconditionally reset the timer to avoid complex branching.
      _times.at(i) = msg.full;
      std::cout << "Timer " << msg.val << " fired.\n";
      msg.nReps -= ( msg.nReps > 0 );
      if ( msg.nReps == 0 ) {  // -1 repeats forever til disabled
        stop( i );
      }
    }
  }
}

void Timer::pause( const unsigned timerId ) {
  _decrementers.at(timerId) = 0;
}

void Timer::unpause( const unsigned timerId ) {
  _decrementers.at(timerId) = 1;
}

auto Timer::start( const unsigned timeMs, const Entity entity, const std::string& timeoutMsg, const int nReps ) -> unsigned {
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
    _msgs.at(timerId)  = TimeoutMsg{ entity, timeoutMsg, nReps, nFrames };
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
  const auto idx = _availableTimers._Find_first();
  _availableTimers.set( idx, 0 );
  return idx;
}
