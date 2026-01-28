#include "Entity.h"

void Entity::validate() {
  personality.validate();
}

void Entity::onInput( const int input ) {
  (*bb)["input"] = std::make_any<char>( input );
  personality.trigger( "onInput" );
}

void Entity::onTimer( const std::string& timerId ) {
  personality.trigger( timerId );
}

void Entity::onCollision( const int collisionType ) {
  (*bb)["collsion"] = collisionType;
  personality.trigger( "onCollision" );
}

