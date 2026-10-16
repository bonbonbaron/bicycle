#include "c/Kinematics.h"
#include "m/World.h"
#include "trigLUT.h"
#include "Constants.h"
#include <cmath>
#include <algorithm>

auto Kinematics::getInstance() -> Kinematics& {
  static Kinematics kin{};
  return kin;
}

// TODO implement fixed point math here
void Kinematics::run() {
  auto& boxes = World::get<Box>();
  // Trackers need to know where they're going before we move them.
  for ( Entity follower = 0; follower < _tracks.size(); ++follower ) {
    auto& target = _tracks.at( follower );
    if ( target == 0 || target == follower ) {
      continue;  // not tracking anything
    }
    auto& trackerPos = World::get<Box>( follower ).pos;
    auto& targetPos = boxes.at( target ).pos;
    // If motion is acceleration-based, accelerate toward target.
    auto& trkrAcc = _accs.at( follower ).pos;
    auto& trkrVel = _vels.at( follower ).pos;
    // unit vector from tracker to target
    if ( ( targetPos - trackerPos ).hasZeroMag() ) {
      continue;  // already there; normalizing would divide by zero
    }
    auto direction = ( targetPos - trackerPos ).normalize();  
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
      trackerPos += _vels.at( target ).pos;
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
  runDriven();
}

// --- scripted motions -----------------------------------------------------------------------------------------------

namespace {
  constexpr float ONE = static_cast<float>( 1 << FIXEDPT_DEC_BITS );
  constexpr float DT = static_cast<float>( Constants::MILLISECONDS_PER_FRAME / 1000.0 );
  auto cellsOf( const Box& b ) -> motion::Vec2 { return { b.pos.x / ONE, b.pos.y / ONE }; }
  auto centerOf( const Box& b ) -> motion::Vec2 {
    return { ( b.pos.x + static_cast<int>( b.size.w ) / 2 ) / ONE, ( b.pos.y + static_cast<int>( b.size.h ) / 2 ) / ONE };
  }
  auto halfSize( const Box& b ) -> motion::Vec2 { return { b.size.w / ( 2 * ONE ), b.size.h / ( 2 * ONE ) }; }
  void place( Box& b, motion::Vec2 topLeft ) {
    b.pos.x = static_cast<int>( std::lround( topLeft.x * ONE ) );
    b.pos.y = static_cast<int>( std::lround( topLeft.y * ONE ) );
  }
}

auto Kinematics::driven( Entity e ) -> Driven& {
  if ( _driven.size() <= e ) _driven.resize( e + 1 );
  return _driven[e];
}

void Kinematics::runDriven() {
  for ( Entity e = 0; e < _driven.size(); ++e ) {
    auto& d = _driven[e];
    if ( !d.on ) continue;
    auto& box = World::get<Box>( e );
    d.t += DT;
    switch ( d.type ) {
      case LINEAR: {
        if ( d.path.empty() ) break;
        bool arrived{};
        d.pos = motion::moveToward( d.pos, d.path[d.idx], d.speed * DT, arrived );
        if ( arrived ) d.idx = motion::nextWaypoint( d.idx, static_cast<int>( d.path.size() ), d.pingPong, d.dir );
        place( box, d.pos );
        break;
      }
      case COMBO:
        if ( !d.path.empty() ) {
          bool arrived{};
          d.pos = motion::moveToward( d.pos, d.path[d.idx], d.speed * DT, arrived );
          if ( arrived ) d.idx = motion::nextWaypoint( d.idx, static_cast<int>( d.path.size() ), d.pingPong, d.dir );
        }
        d.centerPt = d.pos;
        [[fallthrough]];
      case ORBITAL: {
        const motion::Vec2 c = d.center ? centerOf( World::get<Box>( d.center ) ) : d.centerPt;
        const auto p = motion::orbitPoint( c, d.rx, d.ry, motion::orbitAngle( d.phase, d.degPerSec, d.t ) );
        place( box, p - halfSize( box ) );
        break;
      }
      case FOLLOW_LERP: {
        const auto& tb = World::get<Box>( d.tgt );
        const motion::Vec2 tc = centerOf( tb );
        // Hover behind the target: mirror the horizontal offset to the side it's walking away from.
        if ( d.mirror ) {
          if ( tc.x > d.lastTgtX + 0.01f ) d.facing = 1;
          else if ( tc.x < d.lastTgtX - 0.01f ) d.facing = -1;
        }
        d.lastTgtX = tc.x;
        motion::Vec2 goal{ tc.x - ( d.mirror ? d.facing : -1 ) * d.offset.x, tc.y + d.offset.y };
        if ( !d.started ) {
          d.eased = goal;  // the first frame snaps: no swoop in from wherever the entity was spawned
          d.started = true;
        }
        goal = motion::deadZone( d.eased, goal, d.dead );
        d.eased = motion::lerp( d.eased, goal, motion::lerpFactor( d.stiffness, DT ) );
        motion::Vec2 topLeft = motion::Vec2{ d.eased.x, d.eased.y + motion::bob( d.bobRows, d.bobHz, d.t ) } - halfSize( box );
        if ( d.bounded ) {
          // Clamp the eased center too, so leaving a wall doesn't first unwind a backlog.
          topLeft.x = std::clamp( topLeft.x, d.lo.x, d.hi.x );
          topLeft.y = std::clamp( topLeft.y, d.lo.y, d.hi.y );
          d.eased = topLeft + halfSize( box );
        }
        place( box, topLeft );
        break;
      }
    }
  }
}

void Kinematics::linear( Entity e, float speed, bool pingPong ) {
  auto& d = driven( e );
  d.on = true;
  d.type = LINEAR;
  d.speed = speed;
  d.pingPong = pingPong;
  d.idx = 0;
  d.dir = 1;
  d.t = 0;
  d.pos = cellsOf( World::get<Box>( e ) );
  setVelocity( e, Position() );
}

void Kinematics::addWaypoint( Entity e, float x, float y ) {
  driven( e ).path.push_back( { x, y } );
}

void Kinematics::orbit( Entity e, Entity center, float cx, float cy, float rx, float ry, float degPerSec, float phaseDeg ) {
  auto& d = driven( e );
  d.on = true;
  d.type = ORBITAL;
  d.center = center;
  d.centerPt = { cx, cy };
  d.rx = rx;
  d.ry = ry;
  d.degPerSec = degPerSec;
  d.phase = phaseDeg;
  d.t = 0;
  setVelocity( e, Position() );
}

void Kinematics::combo( Entity e, float rx, float ry, float degPerSec, float phaseDeg, float speed, bool pingPong ) {
  auto& d = driven( e );
  d.on = true;
  d.type = COMBO;
  d.center = 0;
  d.rx = rx;
  d.ry = ry;
  d.degPerSec = degPerSec;
  d.phase = phaseDeg;
  d.speed = speed;
  d.pingPong = pingPong;
  d.idx = 0;
  d.dir = 1;
  d.t = 0;
  d.pos = centerOf( World::get<Box>( e ) );  // the orbit starts around where the entity stands
  setVelocity( e, Position() );
}

void Kinematics::follow( Entity e, Entity tgt, float offX, float offY, float rate, float bobRows, float bobHz, bool mirror,
                         float deadX, float deadY ) {
  auto& d = driven( e );
  const bool retarget = d.on && d.type == FOLLOW_LERP;  // switching modes at run time keeps the current position
  d.on = true;
  d.type = FOLLOW_LERP;
  d.tgt = tgt;
  d.offset = { offX, offY };
  d.stiffness = rate;
  d.bobRows = bobRows;
  d.bobHz = bobHz;
  d.mirror = mirror;
  d.dead = d.deadWant = { std::max( 0.f, deadX ), std::max( 0.f, deadY ) };
  if ( !retarget ) {
    d.t = 0;
    d.started = false;
    d.lastTgtX = centerOf( World::get<Box>( tgt ) ).x;
  }
  setVelocity( e, Position() );
}

void Kinematics::capDeadZone( Entity e, float maxX, float maxY ) {
  auto& d = driven( e );
  d.dead = { std::clamp( d.deadWant.x, 0.f, std::max( 0.f, maxX ) ), std::clamp( d.deadWant.y, 0.f, std::max( 0.f, maxY ) ) };
}

void Kinematics::setBounds( Entity e, float minX, float minY, float maxX, float maxY ) {
  auto& d = driven( e );
  d.bounded = true;
  d.lo = { minX, minY };
  d.hi = { std::max( minX, maxX ), std::max( minY, maxY ) };
}

void Kinematics::stopMotion( Entity e ) {
  if ( e < _driven.size() ) {
    _driven[e].on = false;
    _driven[e].path.clear();
  }
}

auto Kinematics::motionType( Entity e ) const -> int {
  return e < _driven.size() && _driven[e].on ? static_cast<int>( _driven[e].type ) : -1;
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

void Kinematics::setVelocity( const Entity entity, const Position& vel ) {
  _vels.at( entity ) = Box();
  _vels.at( entity ).pos = vel;
  _accs.at( entity ) = Box();
  auto& term = _terminalVels.at( entity );
  term.pos.x = std::max( term.pos.x, std::abs( vel.x ) );
  term.pos.y = std::max( term.pos.y, std::abs( vel.y ) );
}

void Kinematics::track( Entity follower, Entity tgt ) {
  _tracks.at( follower ) = tgt;
}

void Kinematics::untrack( Entity follower ) {
  _tracks.at( follower ) = 0;
}
