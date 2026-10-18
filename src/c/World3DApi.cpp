// The true-3D layer's FFI (scripts/World3D.lua wraps it): transforms, bodies, the level, raycasts, camera rigs,
// view settings and positional sound. See include/c/Physics3D.h for units and conventions.
#include "c/Physics3D.h"
#include "c/Kinematics.h"
#include "c/Trigger.h"
#include "c/Collision.h"
#include "c/MidiPlayer.h"
#include "v/common/Renderer.h"
#include "export.h"
#include <cmath>

using phys3d::World3D;

namespace {
void contact( Entity lhs, Entity rhs, unsigned rhsType, bool begin ) {
  Collision c{ lhs, rhs, rhsType };
  if ( begin ) Trigger::sendCollision( c );
  else Trigger::sendUncollision( c );
}
auto w3() -> World3D& {
  static bool hooked = [] { World3D::get().setContactCallback( contact ); return true; }();
  (void)hooked;
  return World3D::get();
}
constexpr float DEG = m3::PI / 180.f;
}

extern "C" {

struct Vec3f { float x, y, z; };
struct RayHit { bool hit; float t, x, y, z, nx, ny, nz; unsigned entity; };

// --- transforms --------------------------------------------------------------------------------------------------------
EXPORT_API void xfSet( Entity e, float x, float y, float z ) {
  auto& t = w3().transform( e );
  t.on = true;
  t.pos = { x, y, z };
}
EXPORT_API Vec3f xfGet( Entity e ) {
  const auto& p = w3().transform( e ).pos;
  return { p.x, p.y, p.z };
}
EXPORT_API void xfSetRot( Entity e, float yawDeg, float pitchDeg, float rollDeg ) {
  auto& t = w3().transform( e );
  t.yaw = yawDeg * DEG;
  t.pitch = pitchDeg * DEG;
  t.roll = rollDeg * DEG;
  t.useQuat = false;
}
EXPORT_API float xfGetYaw( Entity e ) { return w3().transform( e ).yaw / DEG; }
EXPORT_API void xfSetQuat( Entity e, float x, float y, float z, float w ) {
  auto& t = w3().transform( e );
  t.rot = { x, y, z, w };
  t.useQuat = true;
}
EXPORT_API void xfSetScale( Entity e, float x, float y, float z ) { w3().transform( e ).scale = { x, y, z }; }
EXPORT_API void xfSetModelYaw( Entity e, float deg ) { w3().transform( e ).modelYaw = deg * DEG; }
EXPORT_API void xfSetVisible( Entity e, bool v ) { w3().transform( e ).visible = v; }
EXPORT_API void xfSetShadow( Entity e, float radius, float alpha ) {
  auto& t = w3().transform( e );
  t.shadow = radius;
  t.shadowAlpha = alpha;
}
EXPORT_API void xfRemove( Entity e ) {
  Trigger::forgetEntity( e );  // queued events to/from it (we may be inside its own collision dispatch)
  w3().removeEntity( e );
  Kinematics::getInstance().stopMotion( e );
}

// --- bodies ------------------------------------------------------------------------------------------------------------
// kind: 1 character (capsule r, h), 2 kinematic (capsule/sphere: r, h; h = 2r for a sphere), 3 box (half extents
// r = x, h = y, d = z; pos is its center).
EXPORT_API void bodySet( Entity e, int kind, float r, float h, float d, unsigned collType, bool solid ) {
  auto& b = w3().body( e );
  b.kind = static_cast<phys3d::BodyKind>( kind );
  if ( b.kind == phys3d::BOX ) b.half = { r, h, d };
  else { b.radius = r; b.height = std::max( h, 2 * r ); }
  b.collType = collType;
  b.solid = solid;
  w3().transform( e ).on = true;
}
EXPORT_API void bodyTune( Entity e, float gravity, float jumpSpeed, float accelGround, float accelAir, float maxSlopeDeg, float stepHeight ) {
  auto& b = w3().body( e );
  b.gravity = gravity;
  b.jumpSpeed = jumpSpeed;
  b.accelGround = accelGround;
  b.accelAir = accelAir;
  b.maxSlopeCos = std::cos( maxSlopeDeg * DEG );
  b.stepHeight = stepHeight;
}
EXPORT_API void bodyWish( Entity e, float vx, float vz ) {
  auto& b = w3().body( e );
  b.wish = { vx, 0, vz };
  b.steerCam = false;
}
// Camera-relative movement (m/s right and forward of the camera's yaw), kept until changed: Body3D:move.
EXPORT_API void bodyMove( Entity e, float right, float fwd ) {
  auto& b = w3().body( e );
  b.steerCam = true;
  b.moveRight = right;
  b.moveFwd = fwd;
}
EXPORT_API void bodyTurnRate( Entity e, float rate ) { w3().body( e ).turnRate = rate; }
EXPORT_API void bodyJump( Entity e ) { w3().body( e ).jumpReq = true; }
EXPORT_API void bodySetVel( Entity e, float x, float y, float z ) { w3().body( e ).vel = { x, y, z }; }
EXPORT_API Vec3f bodyGetVel( Entity e ) {
  const auto& v = w3().body( e ).vel;
  return { v.x, v.y, v.z };
}
EXPORT_API bool bodyGrounded( Entity e ) { return w3().body( e ).grounded; }
EXPORT_API bool bodyJustLanded( Entity e ) { return w3().body( e ).justLanded; }

// --- level -------------------------------------------------------------------------------------------------------------
EXPORT_API bool levelAddCollision( const char* bmeshPath ) { return w3().addLevelMesh( bmeshPath ); }
EXPORT_API void levelBuild( float cell ) { w3().buildGrid( cell > 0 ? cell : 2.f ); }
EXPORT_API void levelClear() { w3().clearLevel(); }
EXPORT_API unsigned levelTriangles() { return static_cast<unsigned>( w3().levelTriangles() ); }

EXPORT_API RayHit raycast3D( float ox, float oy, float oz, float dx, float dy, float dz, float maxDist, Entity ignore ) {
  const auto h = w3().raycast( { ox, oy, oz }, { dx, dy, dz }, maxDist, ignore, true );
  return { h.hit, h.t, h.point.x, h.point.y, h.point.z, h.normal.x, h.normal.y, h.normal.z, h.entity };
}

// --- 3D motions --------------------------------------------------------------------------------------------------------
EXPORT_API void motionLinear3D( Entity e, float speed, bool pingPong ) { Kinematics::getInstance().linear3D( e, speed, pingPong ); }
EXPORT_API void motionWaypoint3D( Entity e, float x, float y, float z ) { Kinematics::getInstance().addWaypoint3D( e, x, y, z ); }
EXPORT_API void motionOrbit3D( Entity e, Entity center, float cx, float cy, float cz, float r, float degPerSec, float phaseDeg, float bobAmp, float bobHz ) {
  Kinematics::getInstance().orbit3D( e, center, { cx, cy, cz }, r, degPerSec, phaseDeg, bobAmp, bobHz );
}
EXPORT_API void motionView3D( Entity e, Entity tgt, float right, float up, float fwd ) {
  Kinematics::getInstance().view3D( e, tgt, { right, up, fwd } );
}
EXPORT_API void motionKick3D( Entity e, float back, float pitchDeg, float rate ) {
  Kinematics::getInstance().kick3D( e, back, pitchDeg, rate );
}
EXPORT_API void motionFollow3D( Entity e, Entity tgt, float ox, float oy, float oz, float rate ) {
  Kinematics::getInstance().follow3D( e, tgt, { ox, oy, oz }, rate );
}
EXPORT_API void motionSpin3D( Entity e, float degPerSec ) { Kinematics::getInstance().spin3D( e, degPerSec ); }

// --- camera ------------------------------------------------------------------------------------------------------------
// mode: 0 off (2.5D side view of the 2D world), 1 side, 2 orbit (third person), 3 first person, 4 free.
EXPORT_API void cam3dRig( Entity camera, int mode, Entity target ) {
  auto& rig = w3().rig();
  const auto m = static_cast<phys3d::CamMode>( mode );
  if ( m != rig.mode ) rig.started = false;
  rig.mode = m;
  rig.target = target;
  w3().setCameraEntity( camera );
}
EXPORT_API void cam3dParams( float distance, float height, float eyeHeight, float rate, bool collide, float sideDistance ) {
  auto& rig = w3().rig();
  rig.distance = distance;
  rig.height = height;
  rig.eyeHeight = eyeHeight;
  rig.rate = rate;
  rig.collide = collide;
  rig.sideDistance = sideDistance;
}
EXPORT_API void cam3dPitchLimits( float minDeg, float maxDeg ) {
  w3().rig().minPitch = minDeg * DEG;
  w3().rig().maxPitch = maxDeg * DEG;
}
EXPORT_API void cam3dLook( float dYawDeg, float dPitchDeg ) {
  auto& rig = w3().rig();
  rig.yaw += dYawDeg * DEG;
  rig.pitch = std::clamp( rig.pitch + dPitchDeg * DEG, rig.minPitch, rig.maxPitch );
}
EXPORT_API void cam3dSetAngles( float yawDeg, float pitchDeg ) {
  w3().rig().yaw = yawDeg * DEG;
  w3().rig().pitch = pitchDeg * DEG;
}
EXPORT_API void cam3dTurn( float yawDegPerSec, float pitchDegPerSec ) {
  w3().rig().yawRate = yawDegPerSec * DEG;
  w3().rig().pitchRate = pitchDegPerSec * DEG;
}
EXPORT_API float cam3dYaw() { return w3().rig().yaw / DEG; }
EXPORT_API float cam3dPitch() { return w3().rig().pitch / DEG; }
EXPORT_API void cam3dFreeVel( float right, float up, float back ) { w3().rig().freeVel = { right, up, back }; }
// The camera's actual pose (after the rig ran): eye and view angles.
EXPORT_API Vec3f cam3dEye() {
  const auto& p = w3().transform( w3().cameraEntity() ).pos;
  return { p.x, p.y, p.z };
}
EXPORT_API float cam3dViewYaw() { return w3().transform( w3().cameraEntity() ).yaw / DEG; }
EXPORT_API float cam3dViewPitch() { return w3().transform( w3().cameraEntity() ).pitch / DEG; }

// --- view --------------------------------------------------------------------------------------------------------------
EXPORT_API void view3dFov( float deg, float nearZ, float farZ ) {
  auto& v = w3().view();
  v.fovDeg = std::clamp( deg, 10.f, 150.f );
  if ( nearZ > 0 ) v.nearZ = nearZ;
  if ( farZ > v.nearZ ) v.farZ = farZ;
}
EXPORT_API void view3dSky( float r, float g, float b ) { w3().view().sky = { r, g, b }; }
EXPORT_API void view3dFog( float r, float g, float b, float nearZ, float farZ ) {
  auto& v = w3().view();
  v.fogColor = { r, g, b };
  v.fogNear = nearZ;
  v.fogFar = farZ;
}
EXPORT_API void view3dLight( float dx, float dy, float dz, float r, float g, float b, float ambient, bool shadows ) {
  auto& v = w3().view();
  v.lightDir = m3::normalize( { dx, dy, dz } );
  v.lightColor = { r, g, b };
  v.ambient = ambient;
  v.shadows = shadows;
}
EXPORT_API void view3dShadowQuality( int q ) { w3().view().shadowQuality = q < -1 || q > 3 ? -1 : q; }
EXPORT_API void view3dCrosshair( bool on ) { w3().view().crosshair = on; }
EXPORT_API void view3dBillboardPixel( float meters ) { w3().view().billboardPixel = meters; }

// --- sound -------------------------------------------------------------------------------------------------------------
// A sound effect at a point: panned and attenuated for the camera (the listener). ref: full volume within it (m).
EXPORT_API void playSoundFXAt( int key, int vel, float x, float y, float z, float ref, float maxDist ) {
  const auto& cam = w3().transform( w3().cameraEntity() );
  const auto s = m3::spatialize( cam.pos, cam.yaw, { x, y, z }, ref > 0 ? ref : 3.f, maxDist > 0 ? maxDist : 60.f );
  MidiPlayer::getInstance().playSoundFXPanned( key, vel, s.pan, s.gain );
}

}  // extern "C"
