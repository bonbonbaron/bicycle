#include "v/Scene.h"
#include "m/World.h"
#include <cursesw.h>
#include <algorithm>
#include "c/Trigger.h"
#include "v/Image.h"
#include <cassert>

Layer::Layer() : id( newEntityId() ) {}

Layer::Layer( const std::string bgStr, const std::string& bgCollStr, LayerType type ) 
  : id( newEntityId() ) ,
    type( type ) 
{
  World::set<Image>( id, bgStr.c_str() );
  auto& bgImg = World::get<Image>( id );
  World::set<Rect>( id, { { 0, 0, 0 }, bgImg.size } );
  // World::set<Image>( bgCollStr.c_str() ),
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
  std::optional<unsigned> layerIdx{};
  const auto& pair = _entityToLayerMap.find( entity );
  if ( pair != _entityToLayerMap.end() ) {
    layerIdx = pair->second;
  }
  return layerIdx;
}


Scene::Scene() : Window( 0, 0, COLS - 2, LINES ) {
  _camera.setDims( LINES, COLS - 2 );
}

Scene::Scene( const int x, const int y, const int w, const int h ) : Window( x, y, w, h ) {
  _camera.setDims( h, w );
}

void Scene::renderFixedLayer( const Layer& layer ) {
  // Background
  _camera.draw( layer.id, *this );
  // Foreground
  for ( const auto& entity : layer.fg ) {
    _camera.draw( entity, *this );
  }
}

void Scene::render() {
  _camera.followFocus();
  // For each layer..
  for ( const auto& layer : _grid.getLayers() ) {
    // Background
    switch ( layer.type ) {
      case LayerType::FIXED:     // Doesn't move with camera
        renderFixedLayer( layer );
        break;
      case LayerType::GLUED:     // Moves same velocity as camera
        // TODO
        break;
      case LayerType::PARALLAX:  // Moves parallax to camera's focused-on layer
        // TODO
        break;
      case LayerType::AUTOLOOP:  // Moves in constant velocity and loops back
        // TODO
        break;
    }
    // Foreground
    
  }
}

void Scene::onInput( Input& input ) {
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

