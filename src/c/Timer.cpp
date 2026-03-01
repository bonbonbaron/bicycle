#include "c/Timer.h"
#include "m/World.h"

Timer::Timer() : System( "Timer" ) {}

void Timer::run() override {
  auto& times = World::get<Time>();
  const auto nTimes = times.size();
  
  for ( unsigned i = 0; i < nTimes; ++i ) {
    times[i] -= _decrementers[i];
    if ( times[i] == 0 && _decrementers[i] > 0 ) {

  }

}

void Timer::enable( const Entity entity ) {
  const auto& time = World::get<Time>( entity );
  // If we set decrementer to 
  if ( time > 0 ) {
    _decrementers[entity] = 1;
  }
}

void Timer::disable( const Entity entity ) {
  _decrementers[entity] = 0;
}

auto Timer::makeTimer( unsigned timeMs, Entity entity, const std::string& timeoutMsg ) {
  auto timeIdx = findAvailableTimer();

auto Timer::findAvailableTimer() -> unsigned {
  return _timersUsed._Find_first();
}

