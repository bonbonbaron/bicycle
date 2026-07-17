#include "m/Entity.h"
#include  "Bits.h"
#include <bitset>
#include <iostream>

static std::bitset<NUM_SUPPORTED_ENTITIES> _availableEntityIds{-1ULL};

void delEntityId( const Entity entity ) {
  _availableEntityIds.set( entity, true );
}

auto newEntityId() -> Entity {
  // const auto idx = _availableEntityIds._Find_first();  // _Find_first() is a g++ extension. Lucky me.
  const auto idx = find_first_set( _availableEntityIds );
  // std::cout << _availableEntityIds << '\n';
  _availableEntityIds.set( idx, false );
  return idx;
}
