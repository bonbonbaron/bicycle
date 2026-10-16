#pragma once
// Per-entity data in a flat array indexed by entity id (instead of an unordered_map<Entity, T>): one slot per
// supported entity, allocated once from a pool (data/Mem.h). Each slot remembers the entity's generation when it
// was set (entityGeneration, bumped by delEntityId), so a recycled id never sees its previous owner's data: a
// stale slot reads as empty and is reset on the next set.
#include <cstdint>
#include <utility>
#include "data/Mem.h"
#include "m/Entity.h"

template <typename T, mem::PoolId Pool = mem::GENERAL>
class EntitySlots {
  public:
    EntitySlots() : _slots( NUM_SUPPORTED_ENTITIES ) {}

    // The entity's value, or nullptr (never set, erased, or set for an earlier owner of this id).
    auto find( Entity e ) -> T* {
      if ( e >= NUM_SUPPORTED_ENTITIES ) return nullptr;
      auto& s = _slots[e];
      return ( s.used && s.gen == entityGeneration( e ) ) ? &s.value : nullptr;
    }
    auto find( Entity e ) const -> const T* { return const_cast<EntitySlots*>( this )->find( e ); }
    auto contains( Entity e ) const -> bool { return find( e ) != nullptr; }
    // The entity's value, default-constructed if it had none.
    auto operator[]( Entity e ) -> T& {
      if ( T* p = find( e ) ) return *p;
      auto& s = _slots.at( e );
      if ( !s.used ) ++_count;
      s.value = T{};
      s.used = true;
      s.gen = entityGeneration( e );
      return s.value;
    }
    template <typename U>
    auto set( Entity e, U&& v ) -> T& {
      T& slot = ( *this )[e];
      slot = std::forward<U>( v );
      return slot;
    }
    auto erase( Entity e ) -> bool {
      if ( e >= NUM_SUPPORTED_ENTITIES || !_slots[e].used ) return false;
      _slots[e].value = T{};  // releases what it holds (meshes, strings)
      _slots[e].used = false;
      --_count;
      return true;
    }
    void clear() {
      for ( Entity e = 0; e < NUM_SUPPORTED_ENTITIES; ++e ) erase( e );
    }
    // Resets slots whose entity id was freed since they were set (drops what they hold, e.g. mesh references).
    void pruneStale() {
      for ( Entity e = 0; e < NUM_SUPPORTED_ENTITIES; ++e ) {
        if ( _slots[e].used && _slots[e].gen != entityGeneration( e ) ) erase( e );
      }
    }
    auto size() const -> size_t { return _count; }  // slots in use (stale ones included until reset)
    // Calls fn( entity, value ) for every live slot, in entity order.
    template <typename F>
    void forEach( F&& fn ) {
      for ( Entity e = 0; e < NUM_SUPPORTED_ENTITIES; ++e ) {
        if ( T* p = find( e ) ) fn( e, *p );
      }
    }

  private:
    struct Slot {
      T value{};
      unsigned gen{};
      bool used{};
    };
    mem::Vector<Slot, Pool> _slots;
    size_t _count{};
};
