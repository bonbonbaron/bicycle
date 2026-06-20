#include "m/Entity.h"
#include <bitset>

static std::bitset<NUM_SUPPORTED_ENTITIES> _availableEntityIds{1};

void delEntityId( const Entity entity ) {
  _availableEntityIds.set( entity, true );
}

auto newEntityId() -> Entity {
  const auto idx = _availableEntityIds._Find_first();  // _Find_first() is a g++ extension. Lucky me.
  _availableEntityIds.set( idx, false );
  return idx;
}
