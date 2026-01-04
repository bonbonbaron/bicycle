#include "Fallback.h"

auto Fallback::run() -> EventState {
  EventState state{EventState::SUCCESS};  // in case there's nothing in _events
  for ( auto& e : _events ) {
    // Skip already-completed events.
    if ( e->getState() == EventState::SUCCESS ) {
      continue;
    }
    auto state = e->run();
    // Stop on the first event that succeeds.
    if ( state == EventState::SUCCESS ) {
      break;
    }
  }
  setState(state);
  return getState();
}

void Fallback::reset() {
  for ( auto& e : _events ) {
    e->reset();
  }
  setState( EventState::READY );
}
