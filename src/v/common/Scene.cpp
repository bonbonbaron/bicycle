#include "v/common/Scene.h"
#include "c/Physics3D.h"
#include "c/Kinematics.h"
#include "m/World.h"
#include "v/common/Renderer.h"
#include <algorithm>
#include "c/Trigger.h"
#include "v/common/Image.h"
#include <cassert>

Layer::Layer() : id( newEntityId() ) {}

Layer::Layer( const std::string bgStr, const std::string& bgCollStr, LayerType type ) 
  : id( newEntityId() ) ,
    type( type ) 
{
  World::set<Image>( id, bgStr.c_str() );
  auto& bgImg = World::get<Image>( id );
  World::set<Box>( id, { { 0, 0, 0 }, bgImg.size } );
  // World::set<Image>( bgCollStr.c_str() ),
}

Layer::Layer( const std::string& bgStr, LayerType type, int parallaxPct, int parallaxPctY, Position vel, Position offset )
  : id( newEntityId() ),
    type( type ),
    parallaxPct( parallaxPct ),
    parallaxPctY( parallaxPctY ),
    vel( vel )
{
  World::set<Image>( id, bgStr.c_str() );
  auto& bgImg = World::get<Image>( id );
  World::set<Box>( id, { offset, bgImg.size } );
}

Layer::Layer( const Image& img, LayerType type, int parallaxPct, int parallaxPctY, Position vel, Position offset )
  : id( newEntityId() ),
    type( type ),
    parallaxPct( parallaxPct ),
    parallaxPctY( parallaxPctY ),
    vel( vel )
{
  World::set<Image>( id, img );
  World::set<Box>( id, { offset, img.size } );
}

void Grid::addLayer( const Layer& layer ) {
  _layers.push_back( layer );
}

// Adds entity to the latest created layer.
void Grid::addEntity( const Entity entity ) {
  assert( _layers.size() > 0 );
  _layers.back().fg.push_back( entity );
  _entityToLayerMap[ entity ] = _layers.size() - 1;  // so it's zero-indexed
}

void Grid::addEntityTo( const Entity entity, unsigned layerIdx ) {
  assert( _layers.size() > 0 );
  layerIdx = std::min<unsigned>( layerIdx, _layers.size() - 1 );
  _layers[layerIdx].fg.push_back( entity );
  _entityToLayerMap[ entity ] = layerIdx;
}

auto Grid::getLayers() -> std::vector<Layer>& {
  return _layers;
}

void Grid::setFocusedLayerIdx( const unsigned layerIdx ) {
  _focusedLayerIdx = layerIdx;
}

auto Grid::getFocusedLayerIdx() -> unsigned {
  return _focusedLayerIdx;
}

auto Grid::getLayer( Entity entity ) -> std::optional<unsigned> {
  if ( const unsigned* idx = _entityToLayerMap.find( entity ) ) return *idx;
  return std::nullopt;
}


Scene::Scene() : Window( 0, 0, gfx::cols() - 2, gfx::lines() ) {
  _camera.setDims( getHeight(), getWidth() );
  _camera.setMargin( 10 );  // TODO set margin in a bicycle API call
}

Scene::Scene( const int x, const int y, const int w, const int h ) : Window( x, y, w, h ) {
  _camera.setDims( getHeight(), getWidth() );
  _camera.setMargin( 10 );  // TODO set margin in a bicycle API call
}

Scene::~Scene() {
  if ( !World::alive() ) return;  // process exit: the World (and its entity ids) are already gone
  for ( const auto& layer : _grid.getLayers() ) {
    World::set<Image>( layer.id, Image{} );
    World::set<Box>( layer.id, Box{} );
    delEntityId( layer.id );
  }
  const Entity cam = _camera.getId();
  Kinematics::getInstance().forget( cam );
  phys3d::World3D::get().removeEntity( cam );
  World::set<Box>( cam, Box{} );
  delEntityId( cam );
}

// The camera may roam over the largest FIXED layer (the "world"). Other modes move or repeat on their own.
void Scene::updateCameraLimits() {
  int worldW{}, worldH{};
  for ( const auto& layer : _grid.getLayers() ) {
    if ( layer.type == LayerType::FIXED ) {
      const auto& box = World::get<Box>( layer.id );
      worldW = std::max( worldW, box.pos.x + static_cast<int>( box.size.w ) );
      worldH = std::max( worldH, box.pos.y + static_cast<int>( box.size.h ) );
    }
  }
  const auto& camBox = World::get<Box>( _camera.getId() );
  const int maxY = std::max( 0, worldH - static_cast<int>( camBox.size.h ) );
  const int maxX = std::max( 0, worldW - static_cast<int>( camBox.size.w ) );
  _camera.setLims( maxY, maxX );
  // The camera is moved by Kinematics (it tracks its focus like any follower); keep it inside the world.
  constexpr float ONE = 1 << FIXEDPT_DEC_BITS;
  Kinematics::getInstance().setBounds( _camera.getId(), 0, 0, maxX / ONE, maxY / ONE );
}

void Scene::renderFg( const Layer& layer ) {
  for ( const auto& entity : layer.fg ) {
    if ( const bool* h = _hidden.find( entity ); h && *h ) {
      continue;
    }
    _camera.draw( entity, *this );
  }
}

void Scene::render() {
  updateCameraLimits();
  // The camera's position is fixed point (1/256 cell), moved smoothly by Kinematics. Layers are drawn at whole
  // cells (floor) plus a sub-cell remainder that pixel renderers turn into a screen-pixel shift (setLayerSubcell);
  // ASCII ignores it, so everything snaps to the same floored cell and nothing jitters against anything else.
  const auto& camPos = World::get<Box>( _camera.getId() ).pos;
  auto& r = gfx::renderer();
  const int w = getWidth();
  const int h = getHeight();
  r.beginScene( getHandle(), World::get<Box>( _camera.getId() ) );
  constexpr int FRAC = ( 1 << FIXEDPT_DEC_BITS ) - 1;
  // Draws a layer whose top-left window cell shows image position (fx, fy) (fixed point).
  auto draw = [&]( const Layer& layer, const int fx, const int fy, const bool wrap ) {
    r.setLayerSubcell( fx & FRAC, fy & FRAC );
    r.drawLayer( getHandle(), World::get<Image>( layer.id ), fx >> FIXEDPT_DEC_BITS, fy >> FIXEDPT_DEC_BITS, wrap, w, h );
    r.setLayerSubcell( 0, 0 );
  };
  for ( auto& layer : _grid.getLayers() ) {
    const auto& pos = World::get<Box>( layer.id ).pos;
    const int px = static_cast<int>( static_cast<long long>( camPos.x ) * layer.parallaxPct / 100 );
    const int py = static_cast<int>( static_cast<long long>( camPos.y ) * layer.parallaxPctY / 100 );
    switch ( layer.type ) {
      case LayerType::FIXED:     // Part of the world: the camera scrolls over it
        draw( layer, camPos.x - pos.x, camPos.y - pos.y, false );
        break;
      case LayerType::GLUED:     // Moves with the camera, i.e. stays put on screen (skies, HUD art)
        draw( layer, -pos.x, -pos.y, false );
        break;
      case LayerType::PARALLAX:  // Scrolls at parallaxPct of the camera's speed
        draw( layer, px - pos.x, py - pos.y, false );
        break;
      case LayerType::LOOP:      // Like PARALLAX, but repeats endlessly past its edges
        draw( layer, px - pos.x, py - pos.y, true );
        break;
      case LayerType::AUTOLOOP:  // Drifts at a constant velocity and repeats endlessly
        layer.drift += layer.vel;
        draw( layer, px - pos.x + layer.drift.x, py - pos.y + layer.drift.y, true );
        break;
    }
    renderFg( layer );
  }
  r.endScene( getHandle() );
  renderHud();
}

auto Scene::addHudBar( const std::string& label, const int max, const int len, const int x, const int y ) -> int {
  _hud.emplace_back( label, max, len, x, y );
  return static_cast<int>( _hud.size() ) - 1;
}

auto Scene::getHudBar( const int id ) -> Bar* {
  return ( id >= 0 && id < static_cast<int>( _hud.size() ) ) ? &_hud.at( id ) : nullptr;
}

void Scene::setHidden( const Entity entity, const bool hidden ) {
  _hidden[entity] = hidden;
}

void Scene::renderHud() {
  for ( const auto& bar : _hud ) {
    bar.draw( *this );
  }
}

void Scene::onInput( Input& input ) {
}

// A full-screen scene grows or shrinks with the terminal, e.g. when a terminal is maximized after the game started.
// The camera's view follows the window; its limits, dead zone and position are refit right away so the next frame
// (which curses fully repaints after a resize) shows the focus in view.
void Scene::onTermResize() {
  place( 0, 0, std::max( 1, gfx::cols() - 4 ), gfx::lines() );  // same size the default constructor gives
  _camera.setDims( getHeight(), getWidth() );
  updateCameraLimits();
  auto& cam = World::get<Box>( _camera.getId() );
  cam.pos.x = std::clamp<int>( cam.pos.x, 0, _camera.getMaxX() );
  cam.pos.y = std::clamp<int>( cam.pos.y, 0, _camera.getMaxY() );
  if ( const Entity focus = _camera.getFocus() ) {
    // Keep the focus at least 2 cells inside each edge: the dead zone can't be wider than the view allows.
    constexpr float ONE = 1 << FIXEDPT_DEC_BITS;
    const auto& tgt = World::get<Box>( focus );
    Kinematics::getInstance().capDeadZone( _camera.getId(), ( float( cam.size.w ) - tgt.size.w ) / ( 2 * ONE ) - 2,
                                           ( float( cam.size.h ) - tgt.size.h ) / ( 2 * ONE ) - 2 );
  }
}

void Scene::setFocus( Entity entity ) {
  auto layerIdx = _grid.getLayer( entity ); 
  if ( layerIdx.has_value() ) {
    _camera.focusOn( entity );
    _grid.setFocusedLayerIdx( *layerIdx );
  }
}

auto Scene::getGrid() -> Grid& {
  return _grid;
}
