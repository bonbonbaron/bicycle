#include "c/Kinematics.h"
#include "m/World.h"
#include "trigLUT.h"

auto Kinematics::getInstance() -> Kinematics& {
  static Kinematics kin{};
  return kin;
}

// TODO implement fixed point math here
void Kinematics::run() {
  auto& boxes = World::get<Box>();
  // Trackers need to know where they're going before we move them.
  for ( const auto& track : _tracks ) {
    auto& tracker = World::get<Box>( track.tracker ).pos;
    auto& target = boxes.at( track.tgt ).pos;
    // If motion is acceleration-based, accelerate toward target.
    auto& trkrAcc = _accs.at( track.tracker ).pos;
    auto& trkrVel = _vels.at( track.tracker ).pos;
    // unit vector from tracker to target
    auto direction = ( target - tracker).normalize();  
    // Acceleration-based tracking
    if ( ! trkrAcc.hasZeroMag() ) {
      trkrAcc = direction * trkrAcc.mag();
    }
    // Velocity-based tracking
    else if ( ! trkrVel.hasZeroMag() ) {
      trkrVel = direction * trkrVel.mag();
    }
    // Position-based tracking
    else {
      // Since tracker as zero velocity, it just moves with its target.
      tracker += _vels.at( track.tgt ).pos;
    }
  }

  // Add accelerations to velocities.
  for ( unsigned i = 0; i < NUM_SUPPORTED_ENTITIES; ++i ) {
    _vels.at(i) += _accs.at(i);
  }
  // Limit velocities.
  for ( unsigned i = 0; i < NUM_SUPPORTED_ENTITIES; ++i ) {
    auto& vel = _vels.at(i);
    const auto& term = _terminalVels.at(i);
    vel.pos.x = std::clamp( vel.pos.x, -term.pos.x, term.pos.x );
    vel.pos.y = std::clamp( vel.pos.y, -term.pos.y, term.pos.y );
    vel.pos.z = std::clamp( vel.pos.z, -term.pos.z, term.pos.z );
    vel.size.w = std::clamp( vel.size.w, -term.size.w, term.size.w );
    vel.size.h = std::clamp( vel.size.h, -term.size.h, term.size.h );
    vel.size.d = std::clamp( vel.size.d, -term.size.d, term.size.d );
  }
  // Add velocities to positions.
  for ( unsigned i = 0; i < NUM_SUPPORTED_ENTITIES; ++i ) {
    boxes.at(i) += _vels.at(i);
  }
}

void Kinematics::config( const Entity entity, const MotionConfig& src ) {
  auto& dst = _cfgs.at(entity);
  dst.initVel = src.initVel;
  dst.termVel = src.termVel;
  dst.initAcc = src.initAcc;
  dst.type = src.type;
  dst.tgt = src.tgt;
}

void Kinematics::start( const Entity entity ) {
  const auto& cfg = _cfgs.at(entity);
  _vels.at(entity) = cfg.initVel;
  _terminalVels.at(entity) = cfg.termVel;
  _accs.at(entity) = cfg.initAcc;
}

void Kinematics::stop( const Entity entity ) {
  _vels.at(entity) = Box();
  _accs.at(entity) = Box();
}

void Kinematics::pause( const Entity entity ) {
  stop(entity);
}

void Kinematics::unpause( const Entity entity ) {
  start(entity);
}

void Kinematics::track( Entity follower, Entity tgt ) {
  _cfgs.at(follower).tgt = tgt;
}

