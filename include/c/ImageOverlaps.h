#pragma once
// Overlap groups of drawn images, for depth sorting in the flat render modes (2D and ASCII only; 3D has a depth
// buffer and links the no-op hooks in src/c/ImageOverlapsOff.cpp instead of src/c/ImageOverlaps.cpp).
//
// Entities in the same scene layer (same Z) whose image rects (their Boxes, not their collision boxes) overlap,
// directly or through others, form a group. The flat renderers (include/v/common/FlatRenderer.h) sort each group by
// its members' feet and draw everything else in the order it was added. Tracked like collisions, active first: only
// entities that moved since the last update are compared with the rest (CollisionDetector tells which), and only
// layers where an overlap began or ended are regrouped.
#include "data/ActiveFirst.h"
#include "m/Entity.h"
#include <array>
#include <bitset>
#include <vector>

// What the engine calls; no-ops in 3D builds.
namespace imageOverlaps {
void drawnOn( Entity e, Entity layer );  // Scene: e is drawn in that layer's foreground
void moving( Entity e, bool on );        // CollisionDetector::activate / deactivate
void forget( Entity e );                 // despawned
void update();                           // CollisionDetector::check, once a frame
}  // namespace imageOverlaps

class ImageOverlaps {
  public:
    static auto getInstance() -> ImageOverlaps&;
    void add( Entity e, Entity layer );
    void remove( Entity e );
    void setMoving( Entity e, bool on );  // moving: rechecked every update; stopped: once more
    void update();
    auto grouped( Entity layer ) const -> bool { return _layerGrouped.test( layer ); }  // a group of 2+ in it
    auto group( Entity e ) const -> Entity { return _group[e]; }  // its group's id (a member); NONE if alone
    auto overlapping( Entity a, Entity b ) const -> bool { return _overlap.test( a * NUM_SUPPORTED_ENTITIES + b ); }
    static constexpr Entity NONE = ~0u;

  private:
    ImageOverlaps() { _group.fill( NONE ); }
    enum { X0, Y0, X1, Y1, LAYER };  // an image's edges (fixed point; X1/Y1 exclusive) and its layer
    ActiveFirst<int, int, int, int, Entity> _rects;  // active: moved since the last update, or still moving
    std::bitset<NUM_SUPPORTED_ENTITIES * NUM_SUPPORTED_ENTITIES> _overlap{};  // both (a, b) and (b, a)
    std::bitset<NUM_SUPPORTED_ENTITIES> _moving{}, _dirty{}, _layerGrouped{};  // _dirty, _layerGrouped: by layer
    std::array<Entity, NUM_SUPPORTED_ENTITIES> _group{};
    std::vector<uint8_t> _hits;
    void refresh( uint32_t row );
    void setOverlap( Entity a, Entity b, bool on, Entity layer );
    void regroup( Entity layer );
};
