#include "Entity.h"

void Entity::validate() {
  personality.validate();
}

void Entity::onInput( const int input ) {
  try {
    (*bb)["input"] = std::make_any<char>( input );
    personality.trigger( "onInput" );
  }
  catch ( const std::out_of_range& e ) {
    bicycle::die( "Entity with symbol \"" + body.getSymbol() + "\" doesn't have an \"onInput\" trigger." );
  }
}

void Entity::onTimer( const std::string& timerId ) {
  personality.trigger( timerId );
}

void Entity::onCollision( const int collisionType ) {
  (*bb)["collsion"] = collisionType;
  personality.trigger( "onCollision" );
}

