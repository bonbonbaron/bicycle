#include "EventRegistry.h"

void EventRegistry::add( const std::string& name, const std::function<EventState()>& eventFunc ) {
  auto& reg = get();
  reg[ name ] = eventFunc;
}

