// Lua FFI: what entities do over time: animation strips, collision, motion and timers.
#include "LuaApi.h"
#include "c/Animator.h"
#include "c/CollisionDetector.h"
#include "c/Kinematics.h"
#include "c/Timer.h"

namespace {
auto animator() -> Animator& { return Animator::getInstance(); }
auto kinematics() -> Kinematics& { return Kinematics::getInstance(); }
}  // namespace

extern "C" {

// --- animation ------------------------------------------------------------------------------------------------------
// Strips are built once per entity: newAnimStrip(), then addAnimFrame() for each frame in play order. Src rects are
// in character cells within the entity's image (all frames side by side). Strips are keyed by animation tag id
// (ids.anim in gen/ids.lua; Lua resolves names once, scripts/Ids.lua).
EXPORT_API void newAnimStrip( Entity entity, uint8_t stripId, bool repeat ) { animator().newStrip( entity, stripId, repeat ); }
EXPORT_API void addAnimFrame( Entity entity, uint8_t stripId, unsigned durationMs, int x, int y, int w, int h ) {
  animator().addFrame( entity, stripId, durationMs, Rect( x, y, w, h ) );
}
// Switches to and starts a strip; returns false if it's already playing or the entity doesn't have it.
EXPORT_API bool setAnim( Entity entity, uint8_t stripId ) { return animator().set( entity, stripId ); }
EXPORT_API void stopAnim( Entity entity ) { animator().stop( entity ); }
EXPORT_API unsigned getAnimFrameIdx( Entity entity ) { return animator().getFrameIdx( entity ); }

// --- collision ------------------------------------------------------------------------------------------------------
// Registers an entity for collision checks against others on the same layer. x/y/w/h is its collision box relative
// to its position, in cells; w or h of 0 means "use the entity's whole box".
EXPORT_API void addCollider( Entity entity, unsigned layer, unsigned type, int x, int y, int w, int h ) {
  World::set<CollBox>( entity, CollBox( Box( Position( x, y ), Size( w, h ) ), type ) );
  CollisionDetector::registerEntity( entity, layer );
  kinematics().moved( entity );  // checked against everyone on the next frames, wherever it was put
}
// Collision types (a bit each, 1 << type) that stop the entity's steps (stepEntity): coll = { blockedBy = {...} }.
EXPORT_API void setCollBlockers( Entity entity, unsigned typeMask ) {
  CollisionDetector::getInstance().setBlockers( entity, typeMask );
}

// --- moving by hand (fixed point: 256 = one cell) -------------------------------------------------------------------
// One step by dx, dy, minus the parts that go further into a blocker (setCollBlockers) it touches: false (no move) if
// nothing is left. A step into a new one is pushed back out flush in the next collision check (the collision is
// still reported, and stepBlocked() is then true until the next step).
EXPORT_API bool stepEntity( Entity entity, int dx, int dy ) {
  return kinematics().step( entity, lua::fixedPos( dx, dy ) );
}
EXPORT_API bool stepBlocked( Entity entity ) { return CollisionDetector::getInstance().stepBlocked( entity ); }
// Puts the entity at x, y (its box's top-left), no blocking (respawns, knockbacks).
EXPORT_API void teleport( Entity entity, int x, int y ) { kinematics().teleport( entity, lua::fixedPos( x, y ) ); }

// --- motion (cells, seconds, degrees; see Kinematics.h and the motion gene in scripts/Genome.lua) -------------------
// Constant velocity in fixed-point cells per frame (256 = one cell per frame).
EXPORT_API void setVelocity( Entity entity, int vx, int vy ) { kinematics().setVelocity( entity, lua::velocity( vx, vy ) ); }
EXPORT_API void motionLinear( Entity entity, float speed, bool pingPong ) { kinematics().linear( entity, speed, pingPong ); }
EXPORT_API void motionWaypoint( Entity entity, float x, float y ) { kinematics().addWaypoint( entity, x, y ); }
EXPORT_API void motionOrbit( Entity entity, Entity center, float cx, float cy, float rx, float ry, float degPerSec, float phaseDeg ) {
  kinematics().orbit( entity, center, cx, cy, rx, ry, degPerSec, phaseDeg );
}
EXPORT_API void motionCombo( Entity entity, float rx, float ry, float degPerSec, float phaseDeg, float speed, bool pingPong ) {
  kinematics().combo( entity, rx, ry, degPerSec, phaseDeg, speed, pingPong );
}
EXPORT_API void motionFollow( Entity entity, Entity target, float offX, float offY, float rate, float bobRows, float bobHz,
                              bool mirror, float deadX, float deadY ) {
  kinematics().follow( entity, target, offX, offY, rate, bobRows, bobHz, mirror, deadX, deadY );
}
EXPORT_API void motionStop( Entity entity ) { kinematics().stopMotion( entity ); }
EXPORT_API int motionType( Entity entity ) { return kinematics().motionType( entity ); }
// Jumps a follower to where it's heading (after a scene change).
EXPORT_API void motionSnap( Entity entity ) { kinematics().snap( entity ); }

// --- timers ---------------------------------------------------------------------------------------------------------
EXPORT_API TimerId createTimer( const unsigned timeMs, Entity entity, const unsigned timeoutType, const bool repeat,
                                const TimeoutAddr addr ) {
  return Timer::getInstance().create( timeMs, entity, timeoutType, repeat, addr );
}
// Stops a timer (createTimer's id) before it times out again.
EXPORT_API void stopTimer( const TimerId timerId ) { Timer::getInstance().stop( timerId ); }

}  // extern "C"
