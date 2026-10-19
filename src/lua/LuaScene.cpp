// Lua FFI: the current scene: its layers and entities, the camera, and changing scenes.
#include "LuaApi.h"
#include "bicycle.h"
#include "Env.h"
#include "c/Animator.h"
#include "c/CollisionDetector.h"
#include "c/Kinematics.h"
#include "c/Physics3D.h"
#include "c/Timer.h"
#include "c/Trigger.h"
#include "data/Mem.h"
#include "v/common/PixelImages.h"
#include "v/common/Renderer.h"
#include <cstdio>

using lua::currentScene;

extern "C" {

EXPORT_API Entity pushScene() {
  return bicycle::push<Scene>();
}

// --- background layers ----------------------------------------------------------------------------------------------
// type is a LayerType; parallax in percent; vx/vy are fixed-point cells per frame (AUTOLOOP); x/y offset in cells.
EXPORT_API void addBgLayer( const char* bgStr ) {
  if ( auto scene = currentScene() ) scene->getGrid().addLayer( Layer{ std::string( bgStr ), "", LayerType::FIXED } );
}

EXPORT_API void addBgLayerEx( const char* bgStr, int type, int parallaxPct, int parallaxPctY, int vx, int vy, int x, int y ) {
  if ( auto scene = currentScene() ) {
    scene->getGrid().addLayer( Layer{ std::string( bgStr ), static_cast<LayerType>( type ), parallaxPct, parallaxPctY,
                                      lua::velocity( vx, vy ), Position( x, y ) } );
  }
}

// A tilemap layer in pixels: tiles[] (row-major, nw x nh) index the tileset strip (tile i at x = i * tileW).
// Returns false if the tileset is missing or there's no scene.
EXPORT_API bool addBgLayerPixels( const char* tilesetPath, const int* tiles, int nw, int nh, int tileW, int tileH,
                                  int type, int parallaxPct, int parallaxPctY, int vx, int vy, int x, int y ) {
  auto set = pixels::load( tilesetPath );
  if ( !set || tileW <= 0 || tileH <= 0 ) return false;
  auto map = pixels::tilemap( *set, tiles, nw, nh, tileW, tileH );
  auto scene = currentScene();
  if ( !scene ) return false;
  scene->getGrid().addLayer( Layer( pixels::asImage( std::make_shared<const BImg>( std::move( map ) ) ),
                                    static_cast<LayerType>( type ), parallaxPct, parallaxPctY, lua::velocity( vx, vy ),
                                    Position( x, y ) ) );
  return true;
}

// --- entities -------------------------------------------------------------------------------------------------------
EXPORT_API Entity newEntity() {
  return newEntityId();
}

// The entity joins the scene's grid (its image, box etc. are already in the World arrays).
EXPORT_API void addFgEntity( Entity entity, const unsigned layerIdx ) {
  if ( auto scene = currentScene() ) scene->getGrid().addEntity( entity );
}

// A persistent entity joins the current scene's layer layerIdx (0-based; clamped to the last layer).
EXPORT_API void attachEntity( Entity entity, unsigned layerIdx ) {
  auto scene = currentScene();
  if ( scene && !scene->getGrid().getLayers().empty() ) scene->getGrid().addEntityTo( entity, layerIdx );
}

EXPORT_API void setPos( Entity entity, Position pos ) {
  World::get<Box>( entity ).pos = pos << FIXEDPT_DEC_BITS;
}

// Not drawn while hidden (e.g. to flash a character).
EXPORT_API void setHidden( Entity entity, bool hidden ) {
  if ( auto scene = currentScene() ) scene->setHidden( entity, hidden );
}

// Everything the engine keeps for an entity, dropped: timers, animation, motion (and followers of it),
// collisions, its 3D model, its components; then its id is free again.
EXPORT_API void despawnEntity( Entity entity ) {
  Trigger::forgetEntity( entity );  // queued collisions/timeouts to or from it: never delivered
  Timer::getInstance().stopEntity( entity );
  Animator::getInstance().forget( entity );
  Kinematics::getInstance().forget( entity );
  CollisionDetector::getInstance().removeEntity( entity );
  phys3d::World3D::get().removeEntity( entity );  // true 3D: transform, body, overlaps
  gfx::renderer().forgetEntity( entity );
  World::set<Image>( entity, Image{} );
  World::set<Box>( entity, Box{} );
  World::set<CollBox>( entity, CollBox{} );
  World::set<Rect>( entity, Rect{} );
  delEntityId( entity );
}

EXPORT_API void sys( const Trigger::Action action, const Trigger::System system, Entity entity ) {
  Trigger::getInstance().sys( action, system, entity );
}

// --- camera ---------------------------------------------------------------------------------------------------------
// The current scene's camera entity (a kinematic body: Camera.track in scripts/bicycle.lua drives it).
EXPORT_API Entity cameraEntity() {
  auto scene = currentScene();
  return scene ? scene->cameraId() : 0;
}

EXPORT_API void focusCamOn( Entity entity ) {
  if ( auto scene = currentScene() ) scene->setFocus( entity );
}

// --- scene changes (scripts/Scene.lua: Scene.goto) ------------------------------------------------------------------
// While on, images, meshes and songs being loaded go to the SCENE pool (mem::memSceneScope).
EXPORT_API void sceneScope( bool on ) {
  mem::memSceneScope( on );
}

// After the old scene's entities and window are gone: the renderer drops what only it used, then SCENE is reset.
// Returns the SCENE bytes still alive (0 = unloaded cleanly; anything else is a leak and keeps the pool).
EXPORT_API long long sceneUnloaded() {
  gfx::renderer().onSceneUnloaded();
  return static_cast<long long>( mem::memSceneReset() );
}

// BICYCLE_SCENE_STATS=1: one line per scene change on stderr (tests/scene_lifecycle_test.sh reads it).
EXPORT_API void sceneStats( const char* event, const char* sceneName, long long leaked ) {
  static const bool on = env::flag( "BICYCLE_SCENE_STATS" );
  if ( !on ) return;
  const auto s = mem::memStats( mem::SCENE ), g = mem::memStats( mem::GRAPHIC ), a = mem::memStats( mem::AUDIO ),
             gen = mem::memStats( mem::GENERAL );
  std::fprintf( stderr, "bicycle scene: %s %s scene_hw=%zu scene_live=%zu leaked=%lld graphic_hw=%zu graphic_live=%zu "
                "audio_live=%zu general_live=%zu\n", event, sceneName, s.highWater, s.inUse, leaked, g.highWater,
                g.inUse, a.inUse, gen.inUse );
}

// A scene transition's progress (kind: see Renderer::setTransition; t 0..1).
EXPORT_API void setTransition( int kind, float t ) {
  gfx::renderer().setTransition( kind, t );
}

}  // extern "C"
