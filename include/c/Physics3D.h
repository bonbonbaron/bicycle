#pragma once
// The true-3D layer: per-entity transforms and bodies, a static level collider, character physics, raycasts and the
// 3D camera's view settings. Opt-in: an entity is 3D once Lua gives it a transform (World3D.lua); 2D games never
// touch any of this and keep their Box/Vec2 path. Kinematics::run() steps it after the 2D motions.
// Units: meters, seconds, radians; Y up; an entity's position is its feet (bottom center of its capsule).
// Memory: fixed arrays over NUM_SUPPORTED_ENTITIES plus level triangles in GENERAL, built at load; a step allocates
// nothing.
#include "m/Math3D.h"
#include "m/Entity.h"
#include "data/Mem.h"
#include <array>
#include <bitset>
#include <string>

namespace phys3d {

using m3::Vec3;

struct Transform {
  bool on{};
  bool visible{ true };
  Vec3 pos{};
  float yaw{}, pitch{}, roll{};  // the entity's facing; rot is rebuilt from them unless useQuat
  m3::Quat rot{};
  bool useQuat{};
  Vec3 scale{ 1, 1, 1 };
  float modelYaw{};  // extra yaw for the drawn model (e.g. Blender characters face +X: -pi/2 makes them face -Z)
};

enum BodyKind : int {
  NONE = 0,
  CHARACTER = 1,  // capsule; gravity; collides with the level, kinematic boxes and solid bodies; Lua steers it (wish)
  KINEMATIC = 2,  // moved by Lua/Kinematics only; a sphere/capsule for overlaps and raycasts (items, targets)
  BOX = 3,        // kinematic axis-aligned box characters stand on and get carried by (moving platforms)
};

struct Body {
  BodyKind kind{ NONE };
  Vec3 vel{};
  float gravity{ 25.f };       // m/s^2 (Mario-ish snappy; 9.8 is floaty for games)
  float radius{ 0.35f }, height{ 1.6f };
  Vec3 half{ 0.5f, 0.5f, 0.5f };  // BOX: half extents (pos is the box center)
  float maxSlopeCos{ 0.64f };  // ~50 degrees: steeper is a wall
  float stepHeight{ 0.35f };
  Vec3 wish{};                 // desired horizontal velocity (m/s), camera-relative steering is Lua's job
  float accelGround{ 60.f }, accelAir{ 18.f };
  float jumpSpeed{ 9.f };
  bool jumpReq{};
  float coyote{};              // seconds since last grounded (jump grace)
  bool grounded{}, wasGrounded{};
  Vec3 groundN{ 0, 1, 0 };
  Entity groundEntity{};       // BOX stood on
  unsigned collType{};         // nonzero: overlaps are reported (Lua onCollision/onUncollision, rhs's type)
  bool solid{};                // characters can't walk through it
  bool justLanded{};
  float landSpeed{};
};

struct Hit {
  bool hit{};
  float t{};
  Vec3 point{}, normal{};
  Entity entity{};  // 0: the level
};

enum class CamMode : int { OFF = 0, SIDE = 1, ORBIT = 2, FIRST_PERSON = 3, FREE = 4 };

// The camera rig's parameters (Kinematics drives the camera entity's transform from them; see Kinematics::cameraRig).
struct CamRig {
  CamMode mode{ CamMode::OFF };
  Entity target{};
  float yaw{}, pitch{ -0.3f };
  float distance{ 6.f }, height{ 1.4f }, eyeHeight{ 1.5f };
  float rate{ 8.f };          // lerp stiffness (1/s); 0 = hard
  float minPitch{ -1.45f }, maxPitch{ 1.2f };
  bool collide{ true };       // push in when the level is between the pivot and the eye
  float sideDistance{ 9.f };  // SIDE: camera at target + (0, height, sideDistance)
  Vec3 freeVel{};             // FREE: camera-space velocity (m/s) set by Lua
  // state
  Vec3 eased{};
  bool started{};
};

struct View {
  float fovDeg{ 60.f }, nearZ{ 0.05f }, farZ{ 300.f };
  Vec3 sky{ 0.45f, 0.65f, 0.90f };
  Vec3 fogColor{ 0.62f, 0.75f, 0.92f };
  float fogNear{ 30.f }, fogFar{ 120.f };
  Vec3 lightDir{ -0.4f, -1.0f, -0.3f };  // direction the light travels
  Vec3 lightColor{ 1.0f, 0.97f, 0.9f };
  float ambient{ 0.38f };
  bool shadows{ true };
  int shadowQuality{ -1 };            // -1 auto (by GPU), 0 off, 1 low, 2 med, 3 high (BICYCLE_SHADOWS overrides)
  float billboardPixel{ 1.f / 16 };  // meters per sprite pixel (billboards)
  bool crosshair{};                  // a center-screen crosshair (first person)
};

// Overlap reports (begin/end) for entities with a collType; the engine routes them to Lua's onCollision.
using ContactFn = void ( * )( Entity lhs, Entity rhs, unsigned rhsType, bool begin );

class World3D {
  public:
    void setContactCallback( ContactFn f ) { _contact = f; }
    static auto get() -> World3D&;
    auto transform( Entity e ) -> Transform& { return _xf.at( e ); }
    auto body( Entity e ) -> Body& { return _body.at( e ); }
    auto rig() -> CamRig& { return _rig; }
    auto view() -> View& { return _view; }
    auto active() const -> bool { return _rig.mode != CamMode::OFF; }  // the renderer draws true 3D
    auto cameraEntity() const -> Entity { return _camera; }
    void setCameraEntity( Entity e ) { _camera = e; }

    // Level collision: the triangles of a mesh (world space). Call buildGrid() after the last one.
    void addLevelTriangles( const float* pos, const uint32_t* tris, size_t nTris );
    auto addLevelMesh( const std::string& bmeshPath ) -> bool;  // packed .bmesh, or a dev build's loose .obj
    void clearLevel();  // frees the level's memory (it lives in GRAPHIC, i.e. the SCENE pool while a scene loads)
    void removeEntity( Entity e );  // transform + body off; ends its overlaps (uncollision to the other side)
    void buildGrid( float cell = 2.f );
    auto levelTriangles() const -> size_t { return _tris.size(); }

    void step( float dt );
    // Closest hit along origin + dir * t, t in [0, maxDist] (dir normalized inside). ignore: an entity to skip.
    auto raycast( Vec3 origin, Vec3 dir, float maxDist, Entity ignore = 0, bool entities = true ) const -> Hit;
    // Moves a character capsule from its position by delta, sliding along the level. Exposed for tests.
    void moveCharacter( Entity e, Vec3 delta );
    void reset();

  private:
    World3D();
    struct Tri { Vec3 a, b, c, n; };
    template <typename F> void forTrisInBox( Vec3 lo, Vec3 hi, F&& f );
    auto resolve( Entity e, Body& b, Transform& t ) -> bool;  // pushes out of the level; true if it touched
    void overlaps();

    std::array<Transform, NUM_SUPPORTED_ENTITIES> _xf{};
    std::array<Body, NUM_SUPPORTED_ENTITIES> _body{};
    mem::Vector<Tri> _tris{ mem::Allocator<Tri>( mem::GRAPHIC ) };
    mem::Vector<uint32_t> _cellStart{ mem::Allocator<uint32_t>( mem::GRAPHIC ) };
    mem::Vector<uint32_t> _cellTris{ mem::Allocator<uint32_t>( mem::GRAPHIC ) };
    mutable mem::Vector<uint32_t> _stamp{ mem::Allocator<uint32_t>( mem::GRAPHIC ) };
    mutable uint32_t _stampId{};
    Vec3 _lo{}, _hi{};
    float _cell{ 2.f };
    int _nx{}, _nz{};
    std::bitset<NUM_SUPPORTED_ENTITIES * NUM_SUPPORTED_ENTITIES> _touching{};
    std::array<Vec3, NUM_SUPPORTED_ENTITIES> _prevPos{};
    CamRig _rig{};
    View _view{};
    Entity _camera{};
    ContactFn _contact{};
    std::array<Vec3, NUM_SUPPORTED_ENTITIES> _boxPrev{};
};

}  // namespace phys3d
