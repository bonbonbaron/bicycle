#include "Sequence.h"
#include "WindowManager.h"

auto Sequence::run() -> EventState {
  EventState state{EventState::SUCCESS};  // in case there's nothing in _events
  for ( auto& e : _events ) {
    auto state = e.run();
    if ( state != EventState::SUCCESS ) {
      break;
    }
  }
  setState(state);
  return getState();
}
