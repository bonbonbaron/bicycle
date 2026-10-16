#pragma once

constexpr unsigned NUM_SUPPORTED_ENTITIES{256};
using Entity = unsigned;
auto newEntityId() -> Entity;
void delEntityId( const Entity entity );
// Bumped every time the id is freed, so per-entity data (EntitySlots) can tell a recycled id from its old owner.
auto entityGeneration( const Entity entity ) -> unsigned;
