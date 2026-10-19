#include "Bits.h"
#include "c/Timer.h"
#include "Constants.h"
#include "c/Trigger.h"

#include <algorithm>
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

// Counts the running timers down (one pass over a contiguous column, which vectorizes), then delivers the ones due,
// in id order.
void Timer::_run() {
  const uint32_t n = _timers.nActive();
  unsigned* __restrict left = _timers.col<LEFT>();
  for ( uint32_t i = 0; i < n; ++i ) left[i] -= 1;
  _due.clear();
  for ( uint32_t i = 0; i < n; ++i ) {
    if ( left[i] == 0 ) _due.push_back( _timers.key( i ) );
  }
  std::sort( _due.begin(), _due.end() );
  for ( const TimerId id : _due ) {
    // A timeout delivered before this one may have stopped this timer, or reused its id for a new one.
    if ( !_timers.isActive( id ) || _timers.at<LEFT>( id ) != 0 ) continue;
    // Reset or free the slot BEFORE delivering: an animation timeout is handled right here (Animator::tick) and
    // makes the next frame's timer, which can take this very slot; freeing it afterwards killed that timer, so
    // animations stopped after a frame or a cycle.
    const Timeout msg = _timers.at<MSG>( id );
    if ( _timers.at<REPEAT>( id ) ) {
      _timers.at<LEFT>( id ) = _timers.at<FULL>( id );
    } else {
      stop( id );  // a one-shot timer's slot is free again
    }
    Trigger::sendTimeout( msg );
  }
}

void Timer::start( const TimerId timerId ) {
  if ( _timers.contains( timerId ) && _timers.at<LEFT>( timerId ) > 0 ) _timers.activate( timerId );
}

void Timer::stop( unsigned timerId ) {
  if ( timerId >= MAX_NUM_TIMERS ) return;
  _timers.remove( timerId );
  _availableTimers.set( timerId, true );
}

void Timer::stopEntity( const Entity entity ) {
  for ( uint32_t row = _timers.size(); row-- > 0; ) {  // backwards: removing swaps in a row already visited
    if ( _timers.col<MSG>()[row].entity == entity ) stop( _timers.key( row ) );
  }
}

void Timer::pause( const TimerId timerId ) { _timers.deactivate( timerId ); }

void Timer::unpause( const TimerId timerId ) { _timers.activate( timerId ); }

auto Timer::create( const unsigned timeMs, Entity entity, const unsigned timeoutType, const bool repeat, const TimeoutAddr addr) -> TimerId {
  auto timerId = findAvailableTimer();
  if ( timerId < MAX_NUM_TIMERS )  {
    unsigned nFrames{ (Constants::FRAMES_PER_SECOND * timeMs) / 1000 };
    if ( nFrames == 0 ) nFrames = 1;  // under a frame: the next frame (0 never fired: the counter wrapped around)
    _timers.add( timerId, nFrames, nFrames, repeat, Timeout{ entity, timerId, timeoutType, addr } );
    _timers.activate( timerId );
  }
  return timerId;
}

auto Timer::findAvailableTimer() -> unsigned {
  const auto idx = find_first_set( _availableTimers );
  if ( idx < MAX_NUM_TIMERS ) _availableTimers.set( idx, false );
  return idx;
}
