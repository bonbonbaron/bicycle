// See include/c/Physics3D.h.
#include "c/Physics3D.h"
#include "data/Mesh.h"
#include "data/Obj.h"
#include "data/Pack.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace phys3d {

using namespace m3;

auto World3D::get() -> World3D& {
  static World3D w;
  return w;
}

World3D::World3D() {
  _tris.reserve( 1024 );
}

void World3D::reset() {
  _xf.fill( {} );
  _body.fill( {} );
  _rig = {};
  _view = {};
  _touching.reset();
  clearLevel();
}

void World3D::clearLevel() {
  // Swap with empty vectors: clear() would keep the capacity (and the SCENE pool couldn't be reset).
  decltype( _tris )( _tris.get_allocator() ).swap( _tris );
  decltype( _cellStart )( _cellStart.get_allocator() ).swap( _cellStart );
  decltype( _cellTris )( _cellTris.get_allocator() ).swap( _cellTris );
  decltype( _stamp )( _stamp.get_allocator() ).swap( _stamp );
  _nx = _nz = 0;
}

void World3D::removeEntity( Entity e ) {
  if ( e == 0 || e >= NUM_SUPPORTED_ENTITIES ) return;
  for ( Entity o = 1; o < NUM_SUPPORTED_ENTITIES; ++o ) {
    if ( o == e ) continue;
    const Entity i = std::min( e, o ), j = std::max( e, o );
    const std::size_t bit = static_cast<std::size_t>( i ) * NUM_SUPPORTED_ENTITIES + j;
    if ( !_touching.test( bit ) ) continue;
    _touching.reset( bit );
    if ( _contact && _body[o].collType ) _contact( o, e, _body[e].collType, false );
  }
  _xf[e] = {};
  _body[e] = {};
  _prevPos[e] = {};
  _boxPrev[e] = {};
}

void World3D::addLevelTriangles( const float* pos, const uint32_t* tris, std::size_t nTris ) {
  for ( std::size_t i = 0; i < nTris; ++i ) {
    auto v = [&]( int k ) { const uint32_t j = tris[3 * i + k]; return Vec3{ pos[3 * j], pos[3 * j + 1], pos[3 * j + 2] }; };
    Tri t{ v( 0 ), v( 1 ), v( 2 ), {} };
    t.n = normalize( cross( t.b - t.a, t.c - t.a ) );
    if ( length( t.n ) < 0.5f ) continue;  // degenerate
    _tris.push_back( t );
  }
}

auto World3D::addLevelMesh( const std::string& path ) -> bool {
  std::optional<MeshData> m;
  {
    mem::TempScope scratch;
    if ( auto bytes = assets::read( path, mem::TEMPORARY ) ) m = mesh::decode( *bytes );
  }
#ifndef BICYCLE_RELEASE
  if ( !m && path.size() > 6 && path.compare( path.size() - 6, 6, ".bmesh" ) == 0 ) {  // dev: the exporter's .obj
    MeshData d;
    std::string err;
    if ( readObj( path.substr( 0, path.size() - 6 ) + ".obj", d, err ) ) m = std::move( d );
  }
#endif
  if ( !m ) {
    std::fprintf( stderr, "bicycle 3D: no collision mesh %s\n", path.c_str() );
    return false;
  }
  addLevelTriangles( m->pos.data(), m->tris.data(), m->nTris() );
  return true;
}

void World3D::buildGrid( float cell ) {
  _cell = cell;
  _cellStart.clear();
  _cellTris.clear();
  if ( _tris.empty() ) { _nx = _nz = 0; return; }
  _lo = { 1e30f, 1e30f, 1e30f };
  _hi = { -1e30f, -1e30f, -1e30f };
  for ( const auto& t : _tris ) {
    for ( const Vec3& p : { t.a, t.b, t.c } ) {
      _lo = { std::min( _lo.x, p.x ), std::min( _lo.y, p.y ), std::min( _lo.z, p.z ) };
      _hi = { std::max( _hi.x, p.x ), std::max( _hi.y, p.y ), std::max( _hi.z, p.z ) };
    }
  }
  _nx = std::max( 1, static_cast<int>( std::ceil( ( _hi.x - _lo.x ) / cell ) ) );
  _nz = std::max( 1, static_cast<int>( std::ceil( ( _hi.z - _lo.z ) / cell ) ) );
  auto range = [&]( const Tri& t, int& x0, int& x1, int& z0, int& z1 ) {
    const float mnx = std::min( { t.a.x, t.b.x, t.c.x } ), mxx = std::max( { t.a.x, t.b.x, t.c.x } );
    const float mnz = std::min( { t.a.z, t.b.z, t.c.z } ), mxz = std::max( { t.a.z, t.b.z, t.c.z } );
    x0 = std::clamp( static_cast<int>( ( mnx - _lo.x ) / cell ), 0, _nx - 1 );
    x1 = std::clamp( static_cast<int>( ( mxx - _lo.x ) / cell ), 0, _nx - 1 );
    z0 = std::clamp( static_cast<int>( ( mnz - _lo.z ) / cell ), 0, _nz - 1 );
    z1 = std::clamp( static_cast<int>( ( mxz - _lo.z ) / cell ), 0, _nz - 1 );
  };
  // Counting sort into a CSR layout: cellStart[c]..cellStart[c+1] index cellTris.
  _cellStart.assign( static_cast<std::size_t>( _nx ) * _nz + 1, 0 );
  for ( const auto& t : _tris ) {
    int x0, x1, z0, z1;
    range( t, x0, x1, z0, z1 );
    for ( int z = z0; z <= z1; ++z )
      for ( int x = x0; x <= x1; ++x ) ++_cellStart[static_cast<std::size_t>( z ) * _nx + x + 1];
  }
  for ( std::size_t i = 1; i < _cellStart.size(); ++i ) _cellStart[i] += _cellStart[i - 1];
  _cellTris.assign( _cellStart.back(), 0 );
  mem::Vector<uint32_t> fill( _cellStart.begin(), _cellStart.end() - 1, mem::Allocator<uint32_t>( mem::TEMPORARY ) );
  for ( uint32_t i = 0; i < _tris.size(); ++i ) {
    int x0, x1, z0, z1;
    range( _tris[i], x0, x1, z0, z1 );
    for ( int z = z0; z <= z1; ++z )
      for ( int x = x0; x <= x1; ++x ) _cellTris[fill[static_cast<std::size_t>( z ) * _nx + x]++] = i;
  }
  _stamp.assign( _tris.size(), 0 );
  _stampId = 0;
}

template <typename F>
void World3D::forTrisInBox( Vec3 lo, Vec3 hi, F&& f ) {
  if ( _nx == 0 ) return;
  if ( hi.x < _lo.x || lo.x > _hi.x || hi.z < _lo.z || lo.z > _hi.z || hi.y < _lo.y || lo.y > _hi.y ) return;
  if ( ++_stampId == 0 ) { std::fill( _stamp.begin(), _stamp.end(), 0 ); _stampId = 1; }
  const int x0 = std::clamp( static_cast<int>( ( lo.x - _lo.x ) / _cell ), 0, _nx - 1 );
  const int x1 = std::clamp( static_cast<int>( ( hi.x - _lo.x ) / _cell ), 0, _nx - 1 );
  const int z0 = std::clamp( static_cast<int>( ( lo.z - _lo.z ) / _cell ), 0, _nz - 1 );
  const int z1 = std::clamp( static_cast<int>( ( hi.z - _lo.z ) / _cell ), 0, _nz - 1 );
  for ( int z = z0; z <= z1; ++z ) {
    for ( int x = x0; x <= x1; ++x ) {
      const std::size_t c = static_cast<std::size_t>( z ) * _nx + x;
      for ( uint32_t k = _cellStart[c]; k < _cellStart[c + 1]; ++k ) {
        const uint32_t i = _cellTris[k];
        if ( _stamp[i] == _stampId ) continue;
        _stamp[i] = _stampId;
        const Tri& t = _tris[i];
        if ( std::max( { t.a.y, t.b.y, t.c.y } ) < lo.y || std::min( { t.a.y, t.b.y, t.c.y } ) > hi.y ) continue;
        f( t );
      }
    }
  }
}

namespace {

// Closest points between segment pq and triangle abc; returns the distance.
auto segmentTriangle( Vec3 p, Vec3 q, Vec3 a, Vec3 b, Vec3 c, Vec3& sp, Vec3& tp ) -> float {
  const float t = rayTriangle( p, q - p, a, b, c );
  if ( t >= 0 && t <= 1 ) {
    sp = tp = p + ( q - p ) * t;
    return 0;
  }
  float best = 1e30f;
  auto consider = [&]( Vec3 s, Vec3 o ) {
    const float d = dot( s - o, s - o );
    if ( d < best ) { best = d; sp = s; tp = o; }
  };
  consider( p, closestOnTriangle( p, a, b, c ) );
  consider( q, closestOnTriangle( q, a, b, c ) );
  Vec3 c1, c2;
  segmentSegment( p, q, a, b, c1, c2 ); consider( c1, c2 );
  segmentSegment( p, q, b, c, c1, c2 ); consider( c1, c2 );
  segmentSegment( p, q, c, a, c1, c2 ); consider( c1, c2 );
  return std::sqrt( best );
}

auto clampToBox( Vec3 p, Vec3 c, Vec3 h ) -> Vec3 {
  return { std::clamp( p.x, c.x - h.x, c.x + h.x ), std::clamp( p.y, c.y - h.y, c.y + h.y ), std::clamp( p.z, c.z - h.z, c.z + h.z ) };
}
auto closestOnSegment( Vec3 p, Vec3 a, Vec3 b ) -> Vec3 {
  const Vec3 ab = b - a;
  const float l = dot( ab, ab );
  return l < 1e-12f ? a : a + ab * std::clamp( dot( p - a, ab ) / l, 0.f, 1.f );
}
// Closest points between segment ab and an AABB (a few alternations converge for convex shapes).
auto segmentBox( Vec3 a, Vec3 b, Vec3 c, Vec3 h, Vec3& sp, Vec3& bp ) -> float {
  sp = closestOnSegment( c, a, b );
  for ( int i = 0; i < 4; ++i ) {
    bp = clampToBox( sp, c, h );
    sp = closestOnSegment( bp, a, b );
  }
  bp = clampToBox( sp, c, h );
  return length( sp - bp );
}
auto rayBox( Vec3 o, Vec3 d, Vec3 c, Vec3 h, Vec3& n ) -> float {
  float tmin = 0, tmax = 1e30f;
  const float oo[3] = { o.x, o.y, o.z }, dd[3] = { d.x, d.y, d.z }, lo[3] = { c.x - h.x, c.y - h.y, c.z - h.z },
              hi[3] = { c.x + h.x, c.y + h.y, c.z + h.z };
  int axis = -1;
  float sign = 0;
  for ( int i = 0; i < 3; ++i ) {
    if ( std::fabs( dd[i] ) < 1e-9f ) {
      if ( oo[i] < lo[i] || oo[i] > hi[i] ) return -1;
      continue;
    }
    float t1 = ( lo[i] - oo[i] ) / dd[i], t2 = ( hi[i] - oo[i] ) / dd[i];
    float s = -1;
    if ( t1 > t2 ) { std::swap( t1, t2 ); s = 1; }
    if ( t1 > tmin ) { tmin = t1; axis = i; sign = s; }
    tmax = std::min( tmax, t2 );
    if ( tmin > tmax ) return -1;
  }
  n = {};
  if ( axis == 0 ) n.x = sign; else if ( axis == 1 ) n.y = sign; else if ( axis == 2 ) n.z = sign;
  return tmin;
}

}  // namespace

auto World3D::resolve( Entity e, Body& b, Transform& t ) -> bool {
  bool touched = false;
  const float r = b.radius;
  for ( int iter = 0; iter < 4; ++iter ) {
    bool pushed = false;
    // One contact at a time: the deepest, so pushes don't fight each other.
    auto contact = [&]( Vec3 nrm, float pen, Entity other ) {
      if ( nrm.y >= b.maxSlopeCos ) {  // floor: push straight up so standing on a slope doesn't slide
        t.pos.y += std::min( pen / nrm.y, pen * 3 );
        b.grounded = true;
        b.groundN = nrm;
        b.groundEntity = other;
        if ( b.vel.y < 0 ) b.vel.y = 0;
      }
      else {
        t.pos += nrm * pen;
        const float vn = dot( b.vel, nrm );
        if ( vn < 0 ) b.vel -= nrm * vn;
      }
      pushed = touched = true;
    };
    const Vec3 p = t.pos + Vec3{ 0, r, 0 }, q = t.pos + Vec3{ 0, std::max( r, b.height - r ), 0 };
    const Vec3 lo{ t.pos.x - r, t.pos.y, t.pos.z - r }, hi{ t.pos.x + r, t.pos.y + b.height, t.pos.z + r };
    float deepest = 0;
    Vec3 dn{};
    Entity dEnt{};
    forTrisInBox( lo, hi, [&]( const Tri& tri ) {
      Vec3 sp, tp;
      const float d = segmentTriangle( p, q, tri.a, tri.b, tri.c, sp, tp );
      if ( d >= r - 1e-4f ) return;
      Vec3 nrm = d > 1e-5f ? ( sp - tp ) * ( 1 / d ) : tri.n;
      if ( dot( nrm, tri.n ) < 0 && d > 1e-5f ) {
        // Behind a one-sided face (e.g. the far side of a thin wall): use the face normal side we came from.
        nrm = -nrm;
      }
      const float pen = r - d;
      if ( pen > deepest ) { deepest = pen; dn = nrm; dEnt = 0; }
    } );
    for ( Entity o = 1; o < NUM_SUPPORTED_ENTITIES; ++o ) {
      if ( o == e || !_xf[o].on ) continue;
      const Body& ob = _body[o];
      if ( ob.kind == BOX ) {
        Vec3 sp, bp;
        const float d = segmentBox( p, q, _xf[o].pos, ob.half, sp, bp );
        if ( d < r - 1e-4f ) {
          Vec3 nrm = d > 1e-5f ? ( sp - bp ) * ( 1 / d ) : Vec3{ 0, 1, 0 };
          if ( r - d > deepest ) { deepest = r - d; dn = nrm; dEnt = o; }
        }
      }
      else if ( ob.solid && ( ob.kind == KINEMATIC || ob.kind == CHARACTER ) ) {
        const Vec3 op = _xf[o].pos + Vec3{ 0, ob.radius, 0 }, oq = _xf[o].pos + Vec3{ 0, std::max( ob.radius, ob.height - ob.radius ), 0 };
        Vec3 c1, c2;
        const float d = std::sqrt( segmentSegment( p, q, op, oq, c1, c2 ) );
        const float rr = r + ob.radius;
        if ( d < rr - 1e-4f ) {
          Vec3 nrm = normalize( Vec3{ c1.x - c2.x, 0, c1.z - c2.z } );
          if ( length( nrm ) < 0.5f ) nrm = { 1, 0, 0 };
          if ( rr - d > deepest ) { deepest = rr - d; dn = nrm; dEnt = o; }
        }
      }
    }
    if ( deepest > 0 ) contact( dn, deepest, dEnt );
    if ( !pushed ) break;
  }
  return touched;
}

void World3D::moveCharacter( Entity e, Vec3 delta ) {
  auto& b = _body[e];
  auto& t = _xf[e];
  const float len = length( delta );
  const int n = std::clamp( static_cast<int>( std::ceil( len / ( 0.4f * b.radius ) ) ), 1, 32 );
  const Vec3 sub = delta * ( 1.f / n );
  const Vec3 subH{ sub.x, 0, sub.z };
  const float subHLen = length( subH );
  for ( int i = 0; i < n; ++i ) {
    const Vec3 start = t.pos;
    const Vec3 velBefore = b.vel;
    const bool groundedBefore = b.grounded || b.wasGrounded;
    t.pos += sub;
    resolve( e, b, t );
    if ( groundedBefore && subHLen > 1e-5f && b.stepHeight > 0 ) {
      const float progress = dot( Vec3{ t.pos.x - start.x, 0, t.pos.z - start.z }, subH ) / ( subHLen * subHLen );
      if ( progress < 0.6f ) {
        // Blocked: try stepping up (stairs, curbs), then settle back down onto what's there.
        const Vec3 keptPos = t.pos, keptVel = b.vel;
        const bool keptGround = b.grounded;
        t.pos = start + Vec3{ 0, b.stepHeight, 0 };
        b.vel = velBefore;
        resolve( e, b, t );
        t.pos += subH;
        resolve( e, b, t );
        b.grounded = false;
        t.pos.y -= b.stepHeight + 0.02f;
        resolve( e, b, t );
        const float p2 = dot( Vec3{ t.pos.x - start.x, 0, t.pos.z - start.z }, subH ) / ( subHLen * subHLen );
        if ( !( b.grounded && p2 > progress + 0.2f && t.pos.y - start.y <= b.stepHeight + 0.01f ) ) {
          t.pos = keptPos;
          b.vel = keptVel;
          b.grounded = keptGround;
        }
        else if ( b.vel.y < 0 ) b.vel.y = 0;
      }
    }
  }
}

void World3D::step( float dt ) {
  // Boxes (moving platforms) were moved by Lua/Kinematics since last step: carry what stands on them.
  std::array<Vec3, NUM_SUPPORTED_ENTITIES> boxDelta{};
  for ( Entity e = 1; e < NUM_SUPPORTED_ENTITIES; ++e ) {
    if ( _body[e].kind != BOX || !_xf[e].on ) continue;
    boxDelta[e] = _xf[e].pos - _boxPrev[e];
    if ( length( boxDelta[e] ) > 5 ) boxDelta[e] = {};  // teleported
    _boxPrev[e] = _xf[e].pos;
  }
  for ( Entity e = 1; e < NUM_SUPPORTED_ENTITIES; ++e ) {
    auto& b = _body[e];
    auto& t = _xf[e];
    if ( b.kind != CHARACTER || !t.on ) continue;
    if ( b.grounded && b.groundEntity && _body[b.groundEntity].kind == BOX ) t.pos += boxDelta[b.groundEntity];
    if ( b.steerCam ) {
      const float y = _rig.yaw;
      b.wish = { std::cos( y ) * b.moveRight - std::sin( y ) * b.moveFwd, 0, -std::sin( y ) * b.moveRight - std::cos( y ) * b.moveFwd };
    }
    if ( b.turnRate > 0 && ( b.wish.x * b.wish.x + b.wish.z * b.wish.z ) > 0.01f ) {
      const float want = std::atan2( -b.wish.x, -b.wish.z );
      t.yaw = wrapAngle( t.yaw + wrapAngle( want - t.yaw ) * std::min( 1.f, b.turnRate * dt ) );
    }
    // Steer the horizontal velocity toward the wish (snappy on the ground, less in the air).
    const float accel = b.grounded ? b.accelGround : b.accelAir;
    Vec3 dv{ b.wish.x - b.vel.x, 0, b.wish.z - b.vel.z };
    const float dl = length( dv ), maxDv = accel * dt;
    if ( dl > maxDv ) dv = dv * ( maxDv / dl );
    b.vel += dv;
    b.coyote = b.grounded ? 0 : b.coyote + dt;
    b.justLanded = false;
    if ( b.jumpReq && b.coyote < 0.12f ) {
      b.vel.y = b.jumpSpeed;
      b.grounded = false;
      b.coyote = 1;
    }
    b.jumpReq = false;
    if ( !b.grounded ) b.vel.y = std::max( b.vel.y - b.gravity * dt, -60.f );
    else if ( b.vel.y < 0 ) b.vel.y = 0;
    b.wasGrounded = b.grounded;
    const float fallSpeed = -b.vel.y;
    b.grounded = false;
    b.groundEntity = 0;
    // Gravity while standing: a small downward probe keeps contact on slopes and down stairs.
    Vec3 delta = b.vel * dt;
    if ( b.wasGrounded && b.vel.y <= 0.1f ) delta.y -= 0.05f;
    moveCharacter( e, delta );
    if ( !b.grounded && b.wasGrounded && b.vel.y <= 0.1f ) {
      // Walked off a ledge, down a slope or stairs: snap down within a step.
      const Hit h = raycast( t.pos + Vec3{ 0, 0.1f, 0 }, { 0, -1, 0 }, b.stepHeight + 0.1f, e, false );
      if ( h.hit && h.normal.y >= b.maxSlopeCos ) {
        t.pos.y = h.point.y;
        b.grounded = true;
        b.groundN = h.normal;
      }
    }
    if ( b.grounded && !b.wasGrounded ) {
      b.justLanded = true;
      b.landSpeed = fallSpeed;
    }
    if ( t.pos.y < -100 ) {  // fell out of the world: Lua can respawn (Body3D.fellOut)
      b.vel = {};
    }
  }
  overlaps();
}

void World3D::overlaps() {
  if ( !_contact ) return;
  for ( Entity i = 1; i < NUM_SUPPORTED_ENTITIES; ++i ) {
    if ( !_xf[i].on || _body[i].kind == NONE ) continue;
    for ( Entity j = i + 1; j < NUM_SUPPORTED_ENTITIES; ++j ) {
      if ( !_xf[j].on || _body[j].kind == NONE ) continue;
      const Body &a = _body[i], &b = _body[j];
      if ( !a.collType && !b.collType ) continue;
      bool now{};
      auto seg = [&]( Entity e, Vec3& p, Vec3& q ) {
        const Body& bb = _body[e];
        p = _xf[e].pos + Vec3{ 0, bb.radius, 0 };
        q = _xf[e].pos + Vec3{ 0, std::max( bb.radius, bb.height - bb.radius ), 0 };
      };
      if ( a.kind == BOX && b.kind == BOX ) continue;
      if ( a.kind == BOX || b.kind == BOX ) {
        const Entity bx = a.kind == BOX ? i : j, cp = bx == i ? j : i;
        Vec3 p, q, sp, bp;
        seg( cp, p, q );
        now = segmentBox( p, q, _xf[bx].pos, _body[bx].half, sp, bp ) < _body[cp].radius + 0.02f;
      }
      else {
        Vec3 p1, q1, p2, q2, c1, c2;
        seg( i, p1, q1 );
        seg( j, p2, q2 );
        const float rr = a.radius + b.radius + 0.02f;
        now = segmentSegment( p1, q1, p2, q2, c1, c2 ) < rr * rr;
      }
      const std::size_t bit = static_cast<std::size_t>( i ) * NUM_SUPPORTED_ENTITIES + j;
      if ( now == _touching.test( bit ) ) continue;
      _touching.set( bit, now );
      if ( a.collType ) _contact( i, j, b.collType, now );
      if ( b.collType ) _contact( j, i, a.collType, now );
    }
  }
}

auto World3D::raycast( Vec3 o, Vec3 dir, float maxDist, Entity ignore, bool entities ) const -> Hit {
  Hit best;
  best.t = maxDist;
  const Vec3 d = normalize( dir );
  if ( length( d ) < 0.5f ) return {};
  auto testTri = [&]( uint32_t i ) {
    if ( _stamp[i] == _stampId ) return;
    _stamp[i] = _stampId;
    const Tri& t = _tris[i];
    const float h = rayTriangle( o, d, t.a, t.b, t.c );
    if ( h >= 0 && h < best.t ) {
      best = { true, h, o + d * h, dot( t.n, d ) > 0 ? -t.n : t.n, 0 };
    }
  };
  if ( _nx > 0 ) {
    if ( ++_stampId == 0 ) { std::fill( _stamp.begin(), _stamp.end(), 0 ); _stampId = 1; }
    // 2D DDA over the XZ grid (Amanatides & Woo), clipped to the grid's bounds.
    float t0 = 0, t1 = maxDist;
    auto clip = [&]( float oo, float dd, float lo, float hi ) {
      if ( std::fabs( dd ) < 1e-9f ) return oo >= lo && oo <= hi;
      float a = ( lo - oo ) / dd, b = ( hi - oo ) / dd;
      if ( a > b ) std::swap( a, b );
      t0 = std::max( t0, a );
      t1 = std::min( t1, b );
      return t0 <= t1;
    };
    if ( clip( o.x, d.x, _lo.x, _hi.x ) && clip( o.z, d.z, _lo.z, _hi.z ) ) {
      const Vec3 s = o + d * t0;
      int cx = std::clamp( static_cast<int>( ( s.x - _lo.x ) / _cell ), 0, _nx - 1 );
      int cz = std::clamp( static_cast<int>( ( s.z - _lo.z ) / _cell ), 0, _nz - 1 );
      const int sx = d.x > 0 ? 1 : -1, sz = d.z > 0 ? 1 : -1;
      const float inf = 1e30f;
      float tdx = std::fabs( d.x ) > 1e-9f ? _cell / std::fabs( d.x ) : inf;
      float tdz = std::fabs( d.z ) > 1e-9f ? _cell / std::fabs( d.z ) : inf;
      float tmx = std::fabs( d.x ) > 1e-9f ? t0 + ( ( _lo.x + ( cx + ( sx > 0 ) ) * _cell ) - s.x ) / d.x : inf;
      float tmz = std::fabs( d.z ) > 1e-9f ? t0 + ( ( _lo.z + ( cz + ( sz > 0 ) ) * _cell ) - s.z ) / d.z : inf;
      for ( int guard = 0; guard < 4096; ++guard ) {
        const std::size_t c = static_cast<std::size_t>( cz ) * _nx + cx;
        for ( uint32_t k = _cellStart[c]; k < _cellStart[c + 1]; ++k ) testTri( _cellTris[k] );
        const float next = std::min( tmx, tmz );
        if ( best.hit && best.t <= next ) break;  // nothing in later cells can be closer
        if ( next > t1 ) break;
        if ( tmx < tmz ) { cx += sx; tmx += tdx; if ( cx < 0 || cx >= _nx ) break; }
        else { cz += sz; tmz += tdz; if ( cz < 0 || cz >= _nz ) break; }
      }
    }
  }
  if ( entities ) {
    for ( Entity e = 1; e < NUM_SUPPORTED_ENTITIES; ++e ) {
      if ( e == ignore || !_xf[e].on || _body[e].kind == NONE ) continue;
      const Body& b = _body[e];
      if ( b.kind == BOX ) {
        Vec3 n;
        const float h = rayBox( o, d, _xf[e].pos, b.half, n );
        if ( h >= 0 && h < best.t ) best = { true, h, o + d * h, n, e };
        continue;
      }
      // Capsule as three spheres (bottom, middle, top).
      for ( float f : { 0.f, 0.5f, 1.f } ) {
        const Vec3 c = _xf[e].pos + Vec3{ 0, b.radius + f * std::max( 0.f, b.height - 2 * b.radius ), 0 };
        const float h = raySphere( o, d, c, b.radius );
        if ( h >= 0 && h < best.t ) best = { true, h, o + d * h, normalize( o + d * h - c ), e };
      }
    }
  }
  if ( !best.hit ) best.t = 0;
  return best;
}

}  // namespace phys3d
