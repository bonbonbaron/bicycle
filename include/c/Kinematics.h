#pragma once
#include "m/Motion.h"
#include "c/Controllable.h"
#include <vector>
#include <array>
#include "m/Entity.h"

class Kinematics : Controllable {
  public:
    static auto getInstance() -> Kinematics&;
    void run();
    void config( const Entity entity, const MotionConfig& src );  // returns the ID of the timer started for caller's future reference
    void start( const Entity entity ) override;  // returns the ID of the timer started for caller's future reference
    void stop( const Entity entity ) override;
    void pause( const Entity entity ) override;
    void unpause( const Entity entity ) override;
    void track( Entity follower, Entity tgt );  // tgt=0 to disable tracking

  private:
    Kinematics() = default;
    Kinematics(const Kinematics&) = delete;
    Kinematics operator=(const Kinematics&) = delete;
    Kinematics(const Kinematics&&) = delete;
    Kinematics operator=(const Kinematics&&) = delete;

    // To streamline processing, linear and orbital accelerations are treated synonymously (x -> tangential, y -> radial (positive-outward), z ignored for 2D but axial vel in 3D).
    // Only the treatment of cartesian positions is distinguished. Orbits must be translated from rotational to cartesian coordinates at that point.
    // We'll use a sine (doubles as cosine) look-up table to streamline that. Debating between 8- and 16-bit.
    std::array<MotionConfig, NUM_SUPPORTED_ENTITIES> _cfgs{};       
    // Vels & accs combine linear/orbital motion w/ scaling (size-changes).
    std::array<Velocity, NUM_SUPPORTED_ENTITIES> _terminalVels{};       
    std::array<Velocity, NUM_SUPPORTED_ENTITIES> _vels{};       
    std::array<Acceleration, NUM_SUPPORTED_ENTITIES> _accs{};
    MotionType type{};
    Entity tgt{};  // orbiting, tracking (e.g. missiles), or just general destination
};
