#pragma once
// 3D math for the true-3D layer (Physics3D, Kinematics' 3D motions and camera rigs, Renderer3D). Header-only and
// free of engine state so tests/math3d_test.cpp can check it. World units are meters, Y up, right-handed
// (-Z forward at yaw 0). Angles are radians unless a name says Deg.
#include <cmath>
#include <array>
#include <algorithm>

namespace m3 {

constexpr float PI = 3.14159265358979f;

struct Vec3 {
  float x{}, y{}, z{};
};
inline auto operator+( Vec3 a, Vec3 b ) -> Vec3 { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline auto operator-( Vec3 a, Vec3 b ) -> Vec3 { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline auto operator-( Vec3 a ) -> Vec3 { return { -a.x, -a.y, -a.z }; }
inline auto operator*( Vec3 a, float k ) -> Vec3 { return { a.x * k, a.y * k, a.z * k }; }
inline auto operator*( float k, Vec3 a ) -> Vec3 { return a * k; }
inline auto operator+=( Vec3& a, Vec3 b ) -> Vec3& { a = a + b; return a; }
inline auto operator-=( Vec3& a, Vec3 b ) -> Vec3& { a = a - b; return a; }
inline auto dot( Vec3 a, Vec3 b ) -> float { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline auto cross( Vec3 a, Vec3 b ) -> Vec3 { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
inline auto length( Vec3 a ) -> float { return std::sqrt( dot( a, a ) ); }
inline auto normalize( Vec3 a ) -> Vec3 {
  const float l = length( a );
  return l > 1e-12f ? a * ( 1.f / l ) : Vec3{};
}
inline auto lerp( Vec3 a, Vec3 b, float t ) -> Vec3 { return a + ( b - a ) * t; }

// Framerate-independent smoothing (same law as motion::lerpFactor).
inline auto lerpFactor( float k, float dt ) -> float { return k <= 0.f ? 1.f : 1.f - std::exp( -k * dt ); }

// Unit quaternion (x, y, z, w).
struct Quat {
  float x{}, y{}, z{}, w{ 1 };
};
inline auto operator*( Quat a, Quat b ) -> Quat {
  return { a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
           a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z };
}
inline auto axisAngle( Vec3 axis, float a ) -> Quat {
  const Vec3 n = normalize( axis );
  const float s = std::sin( a / 2 );
  return { n.x * s, n.y * s, n.z * s, std::cos( a / 2 ) };
}
// Yaw about +Y then pitch about the local +X (FPS convention: yaw 0 looks down -Z, positive pitch looks up).
inline auto fromYawPitch( float yaw, float pitch ) -> Quat {
  return axisAngle( { 0, 1, 0 }, yaw ) * axisAngle( { 1, 0, 0 }, pitch );
}
inline auto rotate( Quat q, Vec3 v ) -> Vec3 {
  const Vec3 u{ q.x, q.y, q.z };
  const Vec3 t = cross( u, v ) * 2.f;
  return v + t * q.w + cross( u, t );
}
// The direction looked along at (yaw, pitch).
inline auto forward( float yaw, float pitch ) -> Vec3 {
  return { -std::sin( yaw ) * std::cos( pitch ), std::sin( pitch ), -std::cos( yaw ) * std::cos( pitch ) };
}
// Ground-plane basis for camera-relative movement: forward and right on XZ.
inline auto flatForward( float yaw ) -> Vec3 { return { -std::sin( yaw ), 0, -std::cos( yaw ) }; }
inline auto flatRight( float yaw ) -> Vec3 { return { std::cos( yaw ), 0, -std::sin( yaw ) }; }
inline auto wrapAngle( float a ) -> float {
  a = std::fmod( a + PI, 2 * PI );
  if ( a < 0 ) a += 2 * PI;
  return a - PI;
}

// --- matrices (column-major, OpenGL) -----------------------------------------------------------------------------------
using Mat4 = std::array<float, 16>;
inline auto identity() -> Mat4 { return { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }; }
inline auto mul( const Mat4& a, const Mat4& b ) -> Mat4 {
  Mat4 r{};
  for ( int c = 0; c < 4; ++c )
    for ( int rr = 0; rr < 4; ++rr )
      for ( int k = 0; k < 4; ++k ) r[c * 4 + rr] += a[k * 4 + rr] * b[c * 4 + k];
  return r;
}
inline auto transformPoint( const Mat4& m, Vec3 p ) -> Vec3 {
  return { m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12], m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
           m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14] };
}
// translation * rotation * scale
inline auto trs( Vec3 t, Quat q, Vec3 s ) -> Mat4 {
  const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z, xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
  const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
  return { ( 1 - 2 * ( yy + zz ) ) * s.x, 2 * ( xy + wz ) * s.x, 2 * ( xz - wy ) * s.x, 0,
           2 * ( xy - wz ) * s.y, ( 1 - 2 * ( xx + zz ) ) * s.y, 2 * ( yz + wx ) * s.y, 0,
           2 * ( xz + wy ) * s.z, 2 * ( yz - wx ) * s.z, ( 1 - 2 * ( xx + yy ) ) * s.z, 0,
           t.x, t.y, t.z, 1 };
}
inline auto perspective( float fovY, float aspect, float n, float f ) -> Mat4 {
  const float t = 1.0f / std::tan( fovY / 2 );
  Mat4 m{};
  m[0] = t / aspect; m[5] = t; m[10] = ( f + n ) / ( n - f ); m[11] = -1; m[14] = 2 * f * n / ( n - f );
  return m;
}
inline auto ortho( float l, float r, float b, float t, float n, float f ) -> Mat4 {
  Mat4 m = identity();
  m[0] = 2 / ( r - l ); m[5] = 2 / ( t - b ); m[10] = -2 / ( f - n );
  m[12] = -( r + l ) / ( r - l ); m[13] = -( t + b ) / ( t - b ); m[14] = -( f + n ) / ( f - n );
  return m;
}
inline auto lookAt( Vec3 eye, Vec3 at, Vec3 up = { 0, 1, 0 } ) -> Mat4 {
  const Vec3 f = normalize( at - eye );
  Vec3 s = normalize( cross( f, up ) );
  if ( length( s ) < 1e-6f ) s = { 1, 0, 0 };  // looking straight up/down
  const Vec3 u = cross( s, f );
  Mat4 m = identity();
  m[0] = s.x; m[4] = s.y; m[8] = s.z;
  m[1] = u.x; m[5] = u.y; m[9] = u.z;
  m[2] = -f.x; m[6] = -f.y; m[10] = -f.z;
  m[12] = -dot( s, eye ); m[13] = -dot( u, eye ); m[14] = dot( f, eye );
  return m;
}

// --- frustum culling ---------------------------------------------------------------------------------------------------
// Six planes (a, b, c, d; inside where a x + b y + c z + d >= 0) of a view-projection matrix (Gribb/Hartmann).
using Planes = std::array<std::array<float, 4>, 6>;
inline auto frustumPlanes( const Mat4& m ) -> Planes {
  Planes p{};
  for ( int i = 0; i < 3; ++i ) {
    for ( int k = 0; k < 4; ++k ) {
      p[2 * i][k] = m[k * 4 + 3] + m[k * 4 + i];
      p[2 * i + 1][k] = m[k * 4 + 3] - m[k * 4 + i];
    }
  }
  for ( auto& pl : p ) {
    const float l = std::sqrt( pl[0] * pl[0] + pl[1] * pl[1] + pl[2] * pl[2] );
    for ( auto& v : pl ) v /= l;
  }
  return p;
}
inline auto sphereVisible( const Planes& p, Vec3 c, float r ) -> bool {
  for ( const auto& pl : p ) {
    if ( pl[0] * c.x + pl[1] * c.y + pl[2] * c.z + pl[3] < -r ) return false;
  }
  return true;
}

// --- geometry queries --------------------------------------------------------------------------------------------------
// Ray vs triangle (Moller-Trumbore): distance along dir (dir need not be unit; t is in its units), or < 0 if missed.
inline auto rayTriangle( Vec3 o, Vec3 d, Vec3 a, Vec3 b, Vec3 c ) -> float {
  const Vec3 e1 = b - a, e2 = c - a, p = cross( d, e2 );
  const float det = dot( e1, p );
  if ( std::fabs( det ) < 1e-9f ) return -1;
  const float inv = 1 / det;
  const Vec3 s = o - a;
  const float u = dot( s, p ) * inv;
  if ( u < 0 || u > 1 ) return -1;
  const Vec3 q = cross( s, e1 );
  const float v = dot( d, q ) * inv;
  if ( v < 0 || u + v > 1 ) return -1;
  const float t = dot( e2, q ) * inv;
  return t >= 0 ? t : -1;
}
// Ray vs sphere: entry distance, or < 0.
inline auto raySphere( Vec3 o, Vec3 d, Vec3 c, float r ) -> float {
  const Vec3 m = o - c;
  const float a = dot( d, d ), b = dot( m, d ), cc = dot( m, m ) - r * r;
  if ( cc > 0 && b > 0 ) return -1;
  const float disc = b * b - a * cc;
  if ( disc < 0 ) return -1;
  const float t = ( -b - std::sqrt( disc ) ) / a;
  return t < 0 ? 0 : t;
}
// Closest point on triangle abc to p (Ericson, Real-Time Collision Detection 5.1.5).
inline auto closestOnTriangle( Vec3 p, Vec3 a, Vec3 b, Vec3 c ) -> Vec3 {
  const Vec3 ab = b - a, ac = c - a, ap = p - a;
  const float d1 = dot( ab, ap ), d2 = dot( ac, ap );
  if ( d1 <= 0 && d2 <= 0 ) return a;
  const Vec3 bp = p - b;
  const float d3 = dot( ab, bp ), d4 = dot( ac, bp );
  if ( d3 >= 0 && d4 <= d3 ) return b;
  const float vc = d1 * d4 - d3 * d2;
  if ( vc <= 0 && d1 >= 0 && d3 <= 0 ) return a + ab * ( d1 / ( d1 - d3 ) );
  const Vec3 cp = p - c;
  const float d5 = dot( ab, cp ), d6 = dot( ac, cp );
  if ( d6 >= 0 && d5 <= d6 ) return c;
  const float vb = d5 * d2 - d1 * d6;
  if ( vb <= 0 && d2 >= 0 && d6 <= 0 ) return a + ac * ( d2 / ( d2 - d6 ) );
  const float va = d3 * d6 - d5 * d4;
  if ( va <= 0 && ( d4 - d3 ) >= 0 && ( d5 - d6 ) >= 0 ) return b + ( c - b ) * ( ( d4 - d3 ) / ( ( d4 - d3 ) + ( d5 - d6 ) ) );
  const float denom = 1 / ( va + vb + vc );
  return a + ab * ( vb * denom ) + ac * ( vc * denom );
}
// Closest points between segments p1q1 and p2q2 (Ericson 5.1.9); returns squared distance.
inline auto segmentSegment( Vec3 p1, Vec3 q1, Vec3 p2, Vec3 q2, Vec3& c1, Vec3& c2 ) -> float {
  const Vec3 d1 = q1 - p1, d2 = q2 - p2, r = p1 - p2;
  const float a = dot( d1, d1 ), e = dot( d2, d2 ), f = dot( d2, r );
  float s{}, t{};
  if ( a <= 1e-9f && e <= 1e-9f ) { c1 = p1; c2 = p2; return dot( c1 - c2, c1 - c2 ); }
  if ( a <= 1e-9f ) { s = 0; t = std::clamp( f / e, 0.f, 1.f ); }
  else {
    const float c = dot( d1, r );
    if ( e <= 1e-9f ) { t = 0; s = std::clamp( -c / a, 0.f, 1.f ); }
    else {
      const float b = dot( d1, d2 ), denom = a * e - b * b;
      s = denom != 0 ? std::clamp( ( b * f - c * e ) / denom, 0.f, 1.f ) : 0.f;
      t = ( b * s + f ) / e;
      if ( t < 0 ) { t = 0; s = std::clamp( -c / a, 0.f, 1.f ); }
      else if ( t > 1 ) { t = 1; s = std::clamp( ( b - c ) / a, 0.f, 1.f ); }
    }
  }
  c1 = p1 + d1 * s;
  c2 = p2 + d2 * t;
  return dot( c1 - c2, c1 - c2 );
}

// --- positional audio --------------------------------------------------------------------------------------------------
// Stereo pan (-1 left .. +1 right) and gain (0..1) of a source at src heard by a listener at lis facing yaw:
// pan = the source direction's component along the listener's right; gain = ref / (ref + max(0, d - ref)) (inverse
// distance, clamped to 1 inside ref), zero past maxDist.
struct Spatial { float pan{}, gain{}; };
inline auto spatialize( Vec3 lis, float yaw, Vec3 src, float ref, float maxDist ) -> Spatial {
  const Vec3 d = src - lis;
  const float dist = length( d );
  if ( dist >= maxDist ) return { 0, 0 };
  const float pan = dist > 1e-4f ? std::clamp( dot( d * ( 1 / dist ), flatRight( yaw ) ), -1.f, 1.f ) : 0.f;
  const float gain = ref / ( ref + std::max( 0.f, dist - ref ) );
  return { pan, gain };
}

}  // namespace m3
