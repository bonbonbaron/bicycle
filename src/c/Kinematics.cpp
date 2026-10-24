#include "c/Kinematics.h"
#include "m/World.h"
#include "trigLUT.h"
#include "Constants.h"
#include "c/Layer3D.h"
#include "c/CollisionDetector.h"
#include <cmath>
#include <algorithm>
#include <type_traits>

auto Kinematics::getInstance() -> Kinematics& {
  static Kinematics kin{};
  return kin;
}

namespace {
// A Box (velocity, acceleration, speed limit) as its six fixed-point lanes x, y, z, w, h, d, so a loop over n of
// them is one counted loop over 6n ints, which vectorizes (a loop over the structs doesn't). The size lanes are
// unsigned in Box; as ints the limits work the same (a size limit of 0 keeps it 0; above 0, std::clamp on unsigned
// had its bounds crossed).
constexpr uint32_t LANES = 6;
static_assert( sizeof( Box ) == LANES * sizeof( int ) && std::is_standard_layout_v<Box> );
template <typename B> auto lanes( B* boxes ) {
  using Int = std::conditional_t<std::is_const_v<B>, const int, int>;  // TODO research this later
  return reinterpret_cast<Int*>( boxes );
}
}  // namespace

// Per frame, over the moving entities only (the active rows of _motion): trackers aim, accelerations and the speed
// limits apply (counted loops over contiguous columns, which vectorize), then the boxes move and scripted motions
// run. Entities whose settle time ran out stop being active.
#include <iostream>
void Kinematics::run() {
  auto& boxes = World::get<Box>();
  const uint32_t n = _motion.nActive();
  // Trackers need to know where they're going before we move them.
  for ( uint32_t i = 0; i < n; ++i ) {
    std::cout << "Moving enetity " << _motion.key( i ) << " with target " << _motion.col<TRACK>()[i] << std::endl;
    const Entity follower = _motion.key( i ), target = _motion.col<TRACK>()[i];
    if ( target == 0 || target == follower ) continue;  // not tracking anything
    auto& trackerPos = boxes.at( follower ).pos;
    const auto& targetPos = boxes.at( target ).pos;
    // If motion is acceleration-based, accelerate toward target.
    auto& trkrAcc = _motion.col<ACC>()[i].pos;
    auto& trkrVel = _motion.col<VEL>()[i].pos;
    if ( ( targetPos - trackerPos ).hasZeroMag() ) continue;  // already there; normalizing would divide by zero
    auto direction = ( targetPos - trackerPos ).normalize();  // unit vector from tracker to target
    if ( ! trkrAcc.hasZeroMag() ) trkrAcc = direction * trkrAcc.mag();       // acceleration-based tracking
    else if ( ! trkrVel.hasZeroMag() ) trkrVel = direction * trkrVel.mag();  // velocity-based tracking
    else if ( _motion.contains( target ) ) trackerPos += _motion.at<VEL>( target ).pos;  // moves with its target
  }
  // Add accelerations to velocities, then limit them: over the active rows' lanes, one flat loop each.
  Velocity* vels = _motion.col<VEL>();
  int* __restrict v = lanes( vels );
  const int* __restrict a = lanes( _motion.col<ACC>() );
  const int* __restrict t = lanes( _motion.col<TERM>() );
  for ( uint32_t i = 0; i < n * LANES; ++i ) v[i] += a[i];
  for ( uint32_t i = 0; i < n * LANES; ++i ) v[i] = std::clamp( v[i], -t[i], t[i] );
  // Add velocities to positions (World's boxes are per entity: a scatter, not a contiguous loop).
  // Entities that can be blocked tell CollisionDetector where they came from, so it can push them back out.
  auto& cd = CollisionDetector::getInstance();
  for ( uint32_t i = 0; i < n; ++i ) {
    Box& box = boxes.at( _motion.key( i ) );
    if ( cd.blocks( _motion.key( i ) ) && !vels[i].pos.hasZeroMag() ) {
      cd.movedFrom( _motion.key( i ), box.pos, false );
    }
    box += vels[i];
  }
  runDriven();
  // The 3D layer (3D builds only; it works only for entities with a transform): physics, then the camera follows.
  layer3d::step( static_cast<float>( Constants::MILLISECONDS_PER_FRAME / 1000.0 ) );
  // Settling entities count down; those that don't move any more go still (backwards: deactivating swaps a row
  // with the last active one, which was already visited).
  uint8_t* settle = _motion.col<SETTLE>();
  for ( uint32_t i = _motion.nActive(); i-- > 0; ) {
    if ( settle[i] > 0 ) --settle[i];
    const Entity e = _motion.key( i );
    if ( !moving( e ) ) {
      _motion.deactivate( e );
      CollisionDetector::getInstance().deactivate( e );
    }
  }
  std::cout << "over" << std::endl;
}

// --- moving or still -----------------------------------------------------------------------------------------------

auto Kinematics::motionRow( Entity e ) -> uint32_t {
  _motion.add( e, Velocity{}, Acceleration{}, Velocity{}, Entity{}, uint8_t{} );
  return _motion.row( e );
}

auto Kinematics::moving( Entity e ) const -> bool {
  if ( e < _driven.size() && ( _driven[e].on || _driven[e].spin != 0 ) ) return true;
  if ( !_motion.contains( e ) ) return false;
  const auto zero = []( const Box& b ) { return b.pos.hasZeroMag() && b.size.w == 0 && b.size.h == 0 && b.size.d == 0; };
  return !zero( _motion.at<VEL>( e ) ) || !zero( _motion.at<ACC>( e ) ) || _motion.at<TRACK>( e ) != 0 ||
         _motion.at<SETTLE>( e ) > 0;
}

void Kinematics::update( Entity e ) {
  motionRow( e );
  std::cout << "sanity check\n";
  if ( moving( e ) ) {
    std::cout << "activating entity " << e << std::endl;
    _motion.activate( e );
    CollisionDetector::getInstance().activate( e );  // (also when it was already moving but just got a collider)
  }
  else if ( _motion.deactivate( e ) ) {
    std::cout << "deactivating entity " << e << std::endl;
    CollisionDetector::getInstance().deactivate( e );
  }
}

void Kinematics::moved( Entity e ) {
  const uint32_t row = motionRow( e );
  _motion.col<SETTLE>()[row] = SETTLE_FRAMES;
  _motion.activate( e );
  CollisionDetector::getInstance().activate( e );
}

void Kinematics::teleport( Entity e, Position pos ) {
  World::get<Box>( e ).pos = pos;
  moved( e );
}

auto Kinematics::step( Entity e, Position delta ) -> bool {
  auto& cd = CollisionDetector::getInstance();
  const Position d = cd.unblocked( e, delta );
  if ( d.hasZeroMag() && !delta.hasZeroMag() ) return false;
  auto& pos = World::get<Box>( e ).pos;
  cd.movedFrom( e, pos, true );
  pos += d;
  moved( e );
  return true;
}

void Kinematics::stopToward( Entity e, Position dir ) {
  if ( !_motion.contains( e ) ) return;
  auto& v = _motion.at<VEL>( e ).pos;
  if ( v.x * dir.x > 0 ) v.x = 0;
  if ( v.y * dir.y > 0 ) v.y = 0;
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

// The moving entities with a scripted motion, in entity order (a follower placed after its target sees where the
// target went this frame, as it always has).
void Kinematics::runDriven() {
  _drivenNow.clear();
  for ( uint32_t i = 0; i < _motion.nActive(); ++i ) {
    if ( _motion.key( i ) < _driven.size() ) _drivenNow.push_back( _motion.key( i ) );
  }
  std::sort( _drivenNow.begin(), _drivenNow.end() );
  for ( const Entity e : _drivenNow ) {
    auto& d = _driven[e];
    if ( d.spin != 0 || ( d.on && d.type >= LINEAR3D ) ) runDriven3D( e, d );  // Kinematics3D.cpp
    if ( !d.on || d.type >= LINEAR3D ) continue;
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
      default: break;
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

void Kinematics::forget( Entity e ) {
  if ( e < _driven.size() ) {
    _driven[e].path.clear();
    _driven[e] = Driven{};
  }
  for ( auto& d : _driven ) {
    if ( d.on && ( ( d.type == FOLLOW_LERP && d.tgt == e ) || ( d.type == ORBITAL && d.center == e ) ) ) d.on = false;
  }
  for ( uint32_t i = 0; i < _motion.size(); ++i ) {
    if ( _motion.col<TRACK>()[i] == e ) _motion.col<TRACK>()[i] = 0;
  }
  _motion.remove( e );
  CollisionDetector::getInstance().deactivate( e );  // (followers that stopped go still in the next run())
}

void Kinematics::snap( Entity e ) {
  if ( e < _driven.size() ) _driven[e].started = false;
}
void Kinematics::stopMotion( Entity e ) {
  if ( e < _driven.size() ) {
    _driven[e].on = false;
    _driven[e].path.clear();
    _driven[e].path3.clear();
  }
  update( e );
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
  motionRow( entity );
  _motion.at<VEL>( entity ) = cfg.initVel;
  _motion.at<TERM>( entity ) = cfg.termVel;
  _motion.at<ACC>( entity ) = cfg.initAcc;
  update( entity );
}

void Kinematics::stop( const Entity entity ) {
  motionRow( entity );
  _motion.at<VEL>( entity ) = Box();
  _motion.at<ACC>( entity ) = Box();
  update( entity );
}

void Kinematics::pause( const Entity entity ) {
  stop(entity);
}

void Kinematics::unpause( const Entity entity ) {
  start(entity);
}

void Kinematics::setVelocity( const Entity entity, const Position& vel ) {
  motionRow( entity );
  _motion.at<VEL>( entity ) = Box();
  _motion.at<VEL>( entity ).pos = vel;
  _motion.at<ACC>( entity ) = Box();
  auto& term = _motion.at<TERM>( entity );
  term.pos.x = std::max( term.pos.x, std::abs( vel.x ) );
  term.pos.y = std::max( term.pos.y, std::abs( vel.y ) );
  update( entity );
}

void Kinematics::setVelX( const Entity entity, const decltype(Position::x)& velX ) {
  motionRow( entity );
  _motion.at<VEL>( entity ).pos.x = velX;
  update( entity );
}

void Kinematics::setVelY( const Entity entity, const decltype(Position::y)& velY ) {
  motionRow( entity );
  _motion.at<VEL>( entity ).pos.y = velY;
  update( entity );
}

void Kinematics::setVelZ( const Entity entity, const decltype(Position::z)& velZ ) {
  motionRow( entity );
  _motion.at<VEL>( entity ).pos.z = velZ;
  update( entity );
}

void Kinematics::track( Entity follower, Entity tgt ) {
  motionRow( follower );
  _motion.at<TRACK>( follower ) = tgt;
  update( follower );
}

void Kinematics::untrack( Entity follower ) { track( follower, 0 ); }