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
// Blocking: a scripted step (Kinematics::step) into something of a type the entity is blocked by (setBlockers) is
// undone by check(): the collision is still reported (a hero walking into an enemy gets hurt), then the entity is
// put back where it stepped from and stepBlocked() says so. A step toward a blocker it already touches doesn't
// happen at all (blockedToward).
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
    auto blockedToward( Entity entity, Position delta ) const -> bool;  // touches a blocker that way
    void stepped( Entity entity, Position from );  // it stepped this frame from `from` (Kinematics::step)
    auto stepBlocked( Entity entity ) const -> bool { return _stepBlocked.test( entity ); }  // its last step undone

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
    void undoBlockedSteps();

    std::array<Layer, NUM_SUPPORTED_COLL_LAYERS> _layers{};
    std::array<uint8_t, NUM_SUPPORTED_ENTITIES> _layersOf{};   // bit l: on layer l
    std::bitset<NUM_SUPPORTED_ENTITIES * NUM_SUPPORTED_ENTITIES> _touching{};
    std::array<uint32_t, NUM_SUPPORTED_ENTITIES> _blockers{};
    std::array<Position, NUM_SUPPORTED_ENTITIES> _stepFrom{};
    std::bitset<NUM_SUPPORTED_ENTITIES> _stepping{}, _stepBlocked{}, _undo{};
    std::vector<uint8_t> _hits;  // check(): overlap flags of one active row with the rows after it
};
