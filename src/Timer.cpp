#include "Timer.h"
#include "bicycle.h"

#include <string>

Timer::Timer( Callback&& callback, const Duration interval, const bool repeat ) :
  _repeat( repeat ),
  _interval( interval ),
  _callback( callback ) 
{
  _thread = std::jthread{ [this] {
      auto next = Clock::now() + _interval;

      while( _running ) {
        if ( !_running ) {
          break;
        }
        try {
          _callback();
        }
        catch ( const std::exception& e ) {
          bicycle::die( "Timer " + std::to_string(_id) + " callback exception: " + e.what() );
        }
        catch ( ... ) {
          bicycle::die( "Timer callback had an unknown exception." );
        }

        if ( ! _repeat ) {
          break;
        }
        std::this_thread::sleep_until( next );
        next += _interval;
      }
    }
  };
}

Timer::~Timer() {
  stop();
}

void Timer::stop() {
  _running = false;
}

void Timer::pause() {
  // TODO (Grok's answer looks too complicated so i don't wanna do it right now)
}

void Timer::resume() {
  // TODO (Grok's answer looks too complicated so i don't wanna do it right now)
}

void Timer::setInterval( const Duration interval ) {
  _interval = interval;
}

