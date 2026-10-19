#pragma once
#include "m/Motion.h"
#include "c/Controllable.h"
#include <vector>
#include <array>
#include "m/Entity.h"
#include <unordered_set>
#include "c/MotionMath.h"
#include "data/Mem.h"
#include "m/Math3D.h"
#include "data/ActiveFirst.h"

class Kinematics : public Controllable {
  public:
    static auto getInstance() -> Kinematics&;
    void run();
    void config( const Entity entity, const MotionConfig& src );  // returns the ID of the timer started for caller's future reference
    void start( const Entity entity ) override;  // returns the ID of the timer started for caller's future reference
    void stop( const Entity entity ) override;
    void pause( const Entity entity ) override;
    void unpause( const Entity entity ) override;
    void track( Entity follower, Entity tgt );  // tgt=0 to disable tracking
    void untrack( Entity follower );
    void setVelocity( const Entity entity, const Position& vel );  // constant velocity (fixed point per frame)  // tgt=0 to disable tracking

    // Scripted motions (Lua: Genome motion gene, see scripts/Genome.lua). Units are cells, seconds and degrees;
    // an entity's position is its box's top-left, while orbits and follows place the entity's center.
    // A driven entity's box position is owned by Kinematics until stopMotion().
    void linear( Entity e, float speed, bool pingPong );           // along waypoints (addWaypoint) at speed cells/s
    void addWaypoint( Entity e, float x, float y );                  // top-left for LINEAR, orbit center for COMBO
    void orbit( Entity e, Entity center, float cx, float cy, float rx, float ry, float degPerSec, float phaseDeg );
    void combo( Entity e, float rx, float ry, float degPerSec, float phaseDeg, float speed, bool pingPong );
    // Tracks tgt: the entity's center goes to tgt's center + offset. rate 0 = hard (snapped every frame), else
    // FOLLOW_LERP with that stiffness (1/s). mirror flips the offset's x to stay behind tgt as it turns.
    // Dead zone (cells, each side): tgt may wander that far from the goal before the follower moves.
    void follow( Entity e, Entity tgt, float offX, float offY, float rate, float bobRows, float bobHz, bool mirror,
                 float deadX, float deadY );
    // Keeps a driven entity's top-left within [minX, maxX] x [minY, maxY] cells (e.g. the camera in its world).
    // Caps a follower's dead zone (cells, each side) without forgetting the one it asked for, e.g. when the camera
    // shrinks with the terminal; a larger cap later gives back up to the requested size.
    void capDeadZone( Entity e, float maxX, float maxY );
    void setBounds( Entity e, float minX, float minY, float maxX, float maxY );
    void stopMotion( Entity e );
    // Scene changes: forget(e) drops every motion of a despawned entity (and stops followers that tracked it);
    // snap(e) makes a follower jump to its goal on the next frame (e.g. a fairy arriving with the hero).
    void forget( Entity e );
    void snap( Entity e );

    // 3D motions of an entity's transform (meters, seconds, degrees; positions are feet / box centers).
    void linear3D( Entity e, float speed, bool pingPong );   // along addWaypoint3D points
    void addWaypoint3D( Entity e, float x, float y, float z );
    // Around center entity (0: the point c) on a horizontal circle of radius r, plus a vertical bob.
    void orbit3D( Entity e, Entity center, m3::Vec3 c, float r, float degPerSec, float phaseDeg, float bobAmp, float bobHz );
    // The entity's position eases to tgt's + offset (rate 0 = hard).
    void follow3D( Entity e, Entity tgt, m3::Vec3 offset, float rate );
    void spin3D( Entity e, float yawDegPerSec );  // continuous yaw (pickups), with any motion or none
    // Rides in tgt's view: offset is (right, up, forward) meters in tgt's yaw/pitch frame (up is world up) and the
    // entity takes tgt's yaw and pitch, every frame after the camera moved (tgt is usually the camera: a held gun).
    void view3D( Entity e, Entity tgt, m3::Vec3 offset );
    // A recoil kick for a VIEW3D rider: pushed back `back` meters and pitched up `pitchDeg`, decaying at rate 1/s.
    void kick3D( Entity e, float back, float pitchDeg, float rate );
    // The 3D camera: a kinematic body driven by the rig in World3D (Physics3D.h) toward its target, like any
    // follower; Lua sets the mode and feeds yaw/pitch, the camera never positions itself.
    void runCamera3D( float dt );
    void runViews3D( float dt );  // VIEW3D riders, after the camera rig
    auto motionType( Entity e ) const -> int;  // -1 if not driven

    // Moves outside the motions above (scripts, scene placement, animation frames resizing a box). Each keeps the
    // entity active for the next two collision checks (see CollisionDetector.h); fixed-point positions.
    void teleport( Entity e, Position pos );
    // A step by delta, minus any component into a blocker it's flush against: false (no move) if nothing is left.
    // A step into a new blocker is pushed back flush by the next CollisionDetector::check().
    auto step( Entity e, Position delta ) -> bool;
    void stopToward( Entity e, Position dir );  // zeroes each velocity component pointing dir's way (blocked)
    void moved( Entity e );           // its box changed some other way
    auto isMoving( Entity e ) const -> bool { return _motion.isActive( e ); }
  private:
    Kinematics() = default;
    Kinematics(const Kinematics&) = delete;
    Kinematics operator=(const Kinematics&) = delete;
    Kinematics(const Kinematics&&) = delete;
    Kinematics operator=(const Kinematics&&) = delete;

    struct Track {
      Entity tracker;
      Entity tgt;
    };

    std::array<MotionConfig, NUM_SUPPORTED_ENTITIES> _cfgs{};  // start()/unpause() values

    // Entities' velocities and accelerations (and size changes), the active rows the moving ones: run() loops over
    // those only. An entity is moving while it has a velocity, an acceleration, a target it tracks or a scripted
    // motion, and for SETTLE frames after a teleport or step. Its collision checks follow (CollisionDetector).
    enum { VEL, ACC, TERM, TRACK, SETTLE };
    static constexpr uint8_t SETTLE_FRAMES = 3;  // a move in frame N's Lua: active for checks N+1 and N+2
    ActiveFirst<Velocity, Acceleration, Velocity, Entity, uint8_t> _motion;
    auto motionRow( Entity e ) -> uint32_t;  // its row, added (still) if it had none
    void update( Entity e );                 // (de)activates e for whether it moves now
    auto moving( Entity e ) const -> bool;
    struct Driven {
      bool on{};
      MotionType type{};
      motion::Vec2 pos{};         // LINEAR: top-left; COMBO: the orbit's center
      mem::Vector<motion::Vec2> path{ mem::Allocator<motion::Vec2>( mem::GENERAL ) };
      int idx{}, dir{ 1 };
      bool pingPong{};
      float speed{};              // cells per second along the path
      Entity center{};            // ORBITAL around an entity (0: around centerPt)
      motion::Vec2 centerPt{};
      float rx{}, ry{}, degPerSec{}, phase{};
      Entity tgt{};               // FOLLOW_LERP
      motion::Vec2 offset{}, eased{};
      float stiffness{}, bobRows{}, bobHz{};
      float t{};                  // seconds since the motion started
      float lastTgtX{};
      int facing{ 1 };
      bool mirror{}, bounded{}, started{};
      motion::Vec2 dead{}, deadWant{}, lo{}, hi{};
      // 3D
      m3::Vec3 pos3{}, center3{}, offset3{};
      mem::Vector<m3::Vec3> path3{ mem::Allocator<m3::Vec3>( mem::GENERAL ) };
      float radius3{}, bobAmp{}, spin{};
      float kickBack{}, kickPitch{}, kickRate{}, kick{};  // VIEW3D recoil: kick 1 -> 0
    };
    void runDriven3D( Entity e, Driven& d );  // a 3D motion or spin (Kinematics3D.cpp; nothing in 2D builds)
    auto driven( Entity e ) -> Driven&;
    void runDriven();
    mem::Vector<Driven> _driven{ mem::Allocator<Driven>( mem::GENERAL ) };
    std::vector<Entity> _drivenNow;  // runDriven(): this frame's
};
