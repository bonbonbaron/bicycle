#pragma once
#include "m/Motion.h"
#include "c/Controllable.h"
#include <vector>

class Kinematics : Controllable {
  public:
    static auto getInstance() -> Timer&;
    static void run();
    void create( const unsigned timeMs, Entity entity, const unsigned timerType, const bool repeat, const TimeoutAddr addr = TimeoutAddr::BRIDGE);  // returns the ID of the timer started for caller's future reference
    void start( const Entity entity ) override;  // returns the ID of the timer started for caller's future reference
    void stop( const Entity entity ) override;
    void pause( const Entity entity ) override;
    void unpause( const entity ) override;
    void track( Entity follower, Entity tgt );

  private:
    Kinematics();
    Kinematics(const Kinematics&) = delete;
    Kinematics operator=(const Kinematics&) = delete;
    Kinematics(const Kinematics&&) = delete;
    Kinematics operator=(const Kinematics&&) = delete;

    // To streamline processing, linear and orbital accelerations are treated synonymously (x -> tangential, y -> radial (positive-outward), z ignored for 2D but axial vel in 3D).
    // Only the treatment of cartesian positions is distinguished. Orbits must be translated from rotational to cartesian coordinates at that point.
    // We'll use a sine (doubles as cosine) look-up table to streamline that. Debating between 8- and 16-bit.
    std::array<Velocity> _vels{};       
    std::array<Acceleration> _accs{};
    std::array<Growth> _scaleVels{};
    std::array<Growth> _scaleAccs{};
    MotionType type{};
    Entity tgt{};  // orbiting, tracking (e.g. missiles), or just general destination
