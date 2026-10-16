#include "c/Kinematics.h"
#include "m/World.h"
#include "trigLUT.h"
#include "Constants.h"
#include "c/Physics3D.h"
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
  // The 3D layer (only does work for entities with a transform): physics, then the camera follows the result.
  auto& w3 = phys3d::World3D::get();
  w3.step( static_cast<float>( Constants::MILLISECONDS_PER_FRAME / 1000.0 ) );
  runCamera3D( static_cast<float>( Constants::MILLISECONDS_PER_FRAME / 1000.0 ) );
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
    if ( d.spin != 0 ) {
      auto& xf = phys3d::World3D::get().transform( e );
      xf.yaw = m3::wrapAngle( xf.yaw + d.spin * DT );
    }
    if ( !d.on ) continue;
    auto& box = World::get<Box>( e );
    d.t += DT;
    auto& xf = phys3d::World3D::get().transform( e );
    switch ( d.type ) {
      case LINEAR3D: {
        if ( d.path3.empty() ) break;
        const m3::Vec3 to = d.path3[d.idx], dl = to - d.pos3;
        const float len = m3::length( dl ), stepLen = d.speed * DT;
        if ( len <= stepLen || len < 1e-6f ) {
          d.pos3 = to;
          d.idx = motion::nextWaypoint( d.idx, static_cast<int>( d.path3.size() ), d.pingPong, d.dir );
        }
        else d.pos3 += dl * ( stepLen / len );
        xf.pos = d.pos3;
        break;
      }
      case ORBITAL3D: {
        const m3::Vec3 c = d.center ? phys3d::World3D::get().transform( d.center ).pos + d.center3 : d.center3;
        const float a = motion::orbitAngle( d.phase, d.degPerSec, d.t ) * m3::PI / 180.f;
        xf.pos = c + m3::Vec3{ d.radius3 * std::cos( a ), motion::bob( d.bobAmp, d.bobHz, d.t ), d.radius3 * std::sin( a ) };
        break;
      }
      case FOLLOW3D: {
        const m3::Vec3 goal = phys3d::World3D::get().transform( d.tgt ).pos + d.offset3;
        if ( !d.started ) { d.pos3 = goal; d.started = true; }
        d.pos3 = m3::lerp( d.pos3, goal, m3::lerpFactor( d.stiffness, DT ) );
        xf.pos = d.pos3 + m3::Vec3{ 0, motion::bob( d.bobAmp, d.bobHz, d.t ), 0 };
        break;
      }
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
    _driven[e].path3.clear();
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

// --- 3D ---------------------------------------------------------------------------------------------------------------

void Kinematics::linear3D( Entity e, float speed, bool pingPong ) {
  auto& d = driven( e );
  d.on = true;
  d.type = LINEAR3D;
  d.speed = speed;
  d.pingPong = pingPong;
  d.idx = 0;
  d.dir = 1;
  d.t = 0;
  d.pos3 = phys3d::World3D::get().transform( e ).pos;
}

void Kinematics::addWaypoint3D( Entity e, float x, float y, float z ) {
  driven( e ).path3.push_back( { x, y, z } );
}

void Kinematics::orbit3D( Entity e, Entity center, m3::Vec3 c, float r, float degPerSec, float phaseDeg, float bobAmp, float bobHz ) {
  auto& d = driven( e );
  d.on = true;
  d.type = ORBITAL3D;
  d.center = center;
  d.center3 = c;
  d.radius3 = r;
  d.degPerSec = degPerSec;
  d.phase = phaseDeg;
  d.bobAmp = bobAmp;
  d.bobHz = bobHz;
  d.t = 0;
}

void Kinematics::follow3D( Entity e, Entity tgt, m3::Vec3 offset, float rate ) {
  auto& d = driven( e );
  const bool retarget = d.on && d.type == FOLLOW3D;
  d.on = true;
  d.type = FOLLOW3D;
  d.tgt = tgt;
  d.offset3 = offset;
  d.stiffness = rate;
  if ( !retarget ) { d.t = 0; d.started = false; }
}

void Kinematics::spin3D( Entity e, float yawDegPerSec ) {
  driven( e ).spin = yawDegPerSec * m3::PI / 180.f;
}

void Kinematics::runCamera3D( float dt ) {
  auto& w3 = phys3d::World3D::get();
  auto& rig = w3.rig();
  const Entity cam = w3.cameraEntity();
  if ( rig.mode == phys3d::CamMode::OFF || cam == 0 ) return;
  auto& cx = w3.transform( cam );
  cx.on = true;
  cx.visible = false;
  rig.pitch = std::clamp( rig.pitch, rig.minPitch, rig.maxPitch );
  rig.yaw = m3::wrapAngle( rig.yaw );
  const bool haveTgt = rig.target != 0 && w3.transform( rig.target ).on;
  const m3::Vec3 tp = haveTgt ? w3.transform( rig.target ).pos : m3::Vec3{};
  auto lookFrom = [&]( m3::Vec3 eye, m3::Vec3 at ) {
    const m3::Vec3 f = m3::normalize( at - eye );
    cx.pos = eye;
    cx.yaw = std::atan2( -f.x, -f.z );
    cx.pitch = std::asin( std::clamp( f.y, -1.f, 1.f ) );
  };
  auto ease = [&]( m3::Vec3 goal ) {
    if ( !rig.started ) { rig.eased = goal; rig.started = true; }
    rig.eased = m3::lerp( rig.eased, goal, m3::lerpFactor( rig.rate, dt ) );
    return rig.eased;
  };
  switch ( rig.mode ) {
    case phys3d::CamMode::SIDE: {  // the classic side view: beside the target, looking along -Z
      const m3::Vec3 pivot = tp + m3::Vec3{ 0, rig.height, 0 };
      const m3::Vec3 eye = ease( pivot + m3::Vec3{ 0, rig.height * 0.8f, rig.sideDistance } );
      lookFrom( eye, m3::Vec3{ eye.x, pivot.y, pivot.z } );
      break;
    }
    case phys3d::CamMode::ORBIT: {  // third person: behind and above at (yaw, pitch), eased; pushed in by walls
      const m3::Vec3 pivot = tp + m3::Vec3{ 0, rig.height, 0 };
      const m3::Vec3 back = -m3::forward( rig.yaw, rig.pitch );
      float dist = rig.distance;
      if ( rig.collide ) {
        const auto h = w3.raycast( pivot, back, rig.distance, rig.target, false );
        if ( h.hit ) dist = std::max( 0.4f, h.t - 0.25f );
      }
      // The pivot follows hard (no lag on the character); the boom length and angle ease.
      m3::Vec3 off = ease( back * dist );
      if ( m3::length( off ) > dist ) off = m3::normalize( off ) * dist;  // never ease through a wall
      lookFrom( pivot + off, pivot );
      break;
    }
    case phys3d::CamMode::FIRST_PERSON: {
      cx.pos = tp + m3::Vec3{ 0, rig.eyeHeight, 0 };
      cx.yaw = rig.yaw;
      cx.pitch = rig.pitch;
      rig.eased = cx.pos;
      break;
    }
    case phys3d::CamMode::FREE: {
      if ( !rig.started ) { rig.eased = cx.pos; rig.started = true; }
      const m3::Vec3 f = m3::forward( rig.yaw, rig.pitch ), r = m3::flatRight( rig.yaw );
      cx.pos += ( f * -rig.freeVel.z + r * rig.freeVel.x + m3::Vec3{ 0, rig.freeVel.y, 0 } ) * dt;
      cx.yaw = rig.yaw;
      cx.pitch = rig.pitch;
      break;
    }
    default: break;
  }
}
