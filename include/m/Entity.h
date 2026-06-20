#pragma once

constexpr unsigned NUM_SUPPORTED_ENTITIES{256};
using Entity = unsigned;
auto newEntityId() -> Entity;
void delEntityId( const Entity entity );
