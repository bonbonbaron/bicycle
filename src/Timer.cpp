#include "Timer.h"

Timer::Timer(const Callback& callback, const Duration interval, const bool repeat ) :
  _repeat( repeat ),
  _interval( interval ),
  _callback( callback ) 
{
  _thread = std::jthread{ [this] {
      auto next = Clock::now() + _interval;

      while( _running ) {
        std::this_thread::sleep_until( next );
        if ( !_running ) {
          break;
        }

        try {
          _callback();
        }
        catch ( const std::exception& e ) {
          std::cerr << "Timer " << _id << " callback exception: " << e.what() << "\n";
        }
        catch ( ... ) {
          std::cerr << "Timer callback had an unknown exception.\n";
        }

        if ( ! _repeat ) {
          break;
        }
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

