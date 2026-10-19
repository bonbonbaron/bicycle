// Kinematics' true-3D half (3D builds only; ASCII and 2D link src/c/Layer3DOff.cpp): motions of an entity's
// transform (include/c/Physics3D.h) and the 3D camera rig.
#include "c/Kinematics.h"
#include "c/Physics3D.h"
#include "Constants.h"
#include <cmath>
#include <algorithm>

namespace {
  constexpr float DT = static_cast<float>( Constants::MILLISECONDS_PER_FRAME / 1000.0 );
}

// One entity's 3D motion this frame (and its spin), from Kinematics::runDriven.
void Kinematics::runDriven3D( Entity e, Driven& d ) {
  if ( d.spin != 0 ) {
    auto& xf = phys3d::World3D::get().transform( e );
    xf.yaw = m3::wrapAngle( xf.yaw + d.spin * DT );
  }
  if ( !d.on || d.type < LINEAR3D ) return;  // 2D motions: Kinematics::runDriven
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
    case VIEW3D: break;  // runViews3D(), after the camera
    case FOLLOW3D: {
      const m3::Vec3 goal = phys3d::World3D::get().transform( d.tgt ).pos + d.offset3;
      if ( !d.started ) { d.pos3 = goal; d.started = true; }
      d.pos3 = m3::lerp( d.pos3, goal, m3::lerpFactor( d.stiffness, DT ) );
      xf.pos = d.pos3 + m3::Vec3{ 0, motion::bob( d.bobAmp, d.bobHz, d.t ), 0 };
      break;
    }
    default: break;
  }
}

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
  update( e );  // moving now
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
  update( e );  // moving now
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
  update( e );  // moving now
}

void Kinematics::view3D( Entity e, Entity tgt, m3::Vec3 offset ) {
  auto& d = driven( e );
  d.on = true;
  d.type = VIEW3D;
  d.tgt = tgt;
  d.offset3 = offset;
  d.t = 0;
  update( e );  // moving now
}

void Kinematics::kick3D( Entity e, float back, float pitchDeg, float rate ) {
  auto& d = driven( e );
  d.kickBack = back;
  d.kickPitch = pitchDeg * m3::PI / 180.f;
  d.kickRate = rate;
  d.kick = 1;
}

void Kinematics::runViews3D( float dt ) {
  auto& w3 = phys3d::World3D::get();
  for ( Entity e = 0; e < _driven.size(); ++e ) {
    auto& d = _driven[e];
    if ( !d.on || d.type != VIEW3D ) continue;
    const auto& v = w3.transform( d.tgt );
    auto& xf = w3.transform( e );
    const float back = d.kick * d.kickBack;
    const m3::Vec3 f = m3::forward( v.yaw, v.pitch ), r = m3::flatRight( v.yaw );
    xf.pos = v.pos + r * d.offset3.x + m3::Vec3{ 0, d.offset3.y, 0 } + f * ( d.offset3.z - back );
    xf.yaw = v.yaw;
    xf.pitch = v.pitch + d.kick * d.kickPitch;
    d.kick = std::max( 0.f, d.kick - d.kickRate * dt );
  }
}

void Kinematics::spin3D( Entity e, float yawDegPerSec ) {
  driven( e ).spin = yawDegPerSec * m3::PI / 180.f;
  update( e );
}

void Kinematics::runCamera3D( float dt ) {
  auto& w3 = phys3d::World3D::get();
  auto& rig = w3.rig();
  const Entity cam = w3.cameraEntity();
  if ( rig.mode == phys3d::CamMode::OFF || cam == 0 ) return;
  auto& cx = w3.transform( cam );
  cx.on = true;
  cx.visible = false;
  rig.yaw += rig.yawRate * dt;
  rig.pitch += rig.pitchRate * dt;
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
