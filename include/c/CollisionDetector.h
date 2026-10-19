#pragma once
// Overlaps between the entities on each collision layer, sent to Lua as onCollision / onUncollision.
//
// Only moving entities can start or stop touching anything, so each layer is an ActiveFirst table
// (include/data/ActiveFirst.h) of collision boxes: the moving ("active") entities first, still ones behind.
// check() pairs every active entity with every entity after it in its layer: two moving ones once, a moving one
// with each still one. Still pairs aren't looked at at all. Kinematics activates an entity when it starts moving
// and deactivates it when it stops; anything else that moves or resizes an entity goes through Kinematics too
// (teleport, step, moved), so it's checked on the next frames.
//
// Blocking: an entity that moved into something of a type it's blocked by (setBlockers), by a scripted step
// (Kinematics::step) or its velocity, is pushed back out by check() along the axis it came in on, flush against the
// blocker; its velocity loses that component if it pointed into the blocker (so it slides along a wall). The
// collision is still reported (a hero walking into an enemy gets hurt), and while the two stay flush they count as
// touching. A step's components into a blocker it already touches are dropped (unblocked); moving away always works.
#include "c/Collision.h"
#include "data/ActiveFirst.h"
#include "m/Entity.h"
#include <array>
#include <bitset>
#include <vector>

constexpr unsigned NUM_SUPPORTED_COLL_LAYERS{8};

class CollisionDetector {
  public:
    static void check();
    static void registerEntity( Entity entity, unsigned layer );
    static auto getInstance() -> CollisionDetector&;
    void addEntity( Entity entity, unsigned layer );
    void removeEntity( Entity entity );  // from every layer, and forgets its ongoing collisions
    void activate( Entity entity );      // it moves: check it against everyone (Kinematics)
    void deactivate( Entity entity );    // it's still: only moving entities are checked against it
    auto isActive( Entity entity ) const -> bool;
    auto touching( Entity a, Entity b ) const -> bool { return _touching.test( pairBit( a, b ) ); }

    void setBlockers( Entity entity, uint32_t collTypeMask );  // bit t: collision type t blocks its steps
    auto blocks( Entity entity ) const -> bool { return _blockers[entity] != 0; }
    auto unblocked( Entity entity, Position delta ) const -> Position;  // delta minus what heads into touched blockers
    // It moves this frame from `from` (Kinematics: steps and velocities of entities with blockers).
    void movedFrom( Entity entity, Position from, bool step );
    auto stepBlocked( Entity entity ) const -> bool { return _stepBlocked.test( entity ); }  // its last step cut short

  private:
    CollisionDetector() = default;
    CollisionDetector(const CollisionDetector&) = delete;
    CollisionDetector operator=(const CollisionDetector&) = delete;
    CollisionDetector(const CollisionDetector&&) = delete;
    CollisionDetector operator=(const CollisionDetector&&) = delete;

    enum { X0, Y0, X1, Y1 };              // a collision box's edges (fixed point; X1/Y1 exclusive)
    using Layer = ActiveFirst<int, int, int, int>;
    static auto pairBit( Entity a, Entity b ) -> size_t {
      return a < b ? a * NUM_SUPPORTED_ENTITIES + b : b * NUM_SUPPORTED_ENTITIES + a;
    }
    void checkLayer( Layer& layer );
    void pairState( Entity a, Entity b, bool overlap );  // sends what changed between a and b
    void refresh( Layer& layer, uint32_t row );          // the row's edges from the entity's Box and CollBox
    auto blocking( Entity a, Entity b ) const -> bool;  // either is blocked by the other's type
    void pushOut( Entity mover, Entity blocker );       // flush against it, along the axis it came in on

    std::array<Layer, NUM_SUPPORTED_COLL_LAYERS> _layers{};
    std::array<uint8_t, NUM_SUPPORTED_ENTITIES> _layersOf{};   // bit l: on layer l
    std::bitset<NUM_SUPPORTED_ENTITIES * NUM_SUPPORTED_ENTITIES> _touching{};
    std::array<uint32_t, NUM_SUPPORTED_ENTITIES> _blockers{};
    std::array<Position, NUM_SUPPORTED_ENTITIES> _from{};       // where it was before this frame's moves
    std::bitset<NUM_SUPPORTED_ENTITIES> _moved{}, _stepping{}, _stepBlocked{};
    std::vector<std::pair<Entity, Entity>> _pushes;            // check(): (mover, blocker) overlaps to resolve
    std::vector<uint8_t> _hits;  // check(): overlap flags of one active row with the rows after it
};
