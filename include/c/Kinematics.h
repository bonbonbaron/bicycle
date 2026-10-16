#pragma once
#include "m/Motion.h"
#include "c/Controllable.h"
#include <vector>
#include <array>
#include "m/Entity.h"
#include <unordered_set>
#include "c/MotionMath.h"
#include "data/Mem.h"

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
    auto motionType( Entity e ) const -> int;  // -1 if not driven
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

    // To streamline processing, linear and orbital accelerations are treated synonymously (x -> tangential, y -> radial (positive-outward), z ignored for 2D but axial vel in 3D).
    // Only the treatment of cartesian positions is distinguished. Orbits must be translated from rotational to cartesian coordinates at that point.
    // We'll use a sine (doubles as cosine) look-up table to streamline that. Debating between 8- and 16-bit.
    // TODO if this underperforms per frame, then I'll make these vectors of active motions.
    std::array<MotionConfig, NUM_SUPPORTED_ENTITIES> _cfgs{};       
    // Vels & accs combine linear/orbital motion w/ scaling (size-changes).
    std::array<Velocity, NUM_SUPPORTED_ENTITIES> _terminalVels{};       
    std::array<Velocity, NUM_SUPPORTED_ENTITIES> _vels{};       
    std::array<Acceleration, NUM_SUPPORTED_ENTITIES> _accs{};
    std::array<Entity, NUM_SUPPORTED_ENTITIES> _tracks{};
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
    };
    auto driven( Entity e ) -> Driven&;
    void runDriven();
    mem::Vector<Driven> _driven{ mem::Allocator<Driven>( mem::GENERAL ) };
    MotionType type{};
    Entity tgt{};  // orbiting, tracking (e.g. missiles), or just general destination
};
