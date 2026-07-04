#include "v/Scene.h"
#include "m/World.h"
#include <cursesw.h>
#include <algorithm>
#include "c/Trigger.h"

Layer::Layer() : id( newEntityId() ) {}

Layer::Layer( const std::string bgStr, const std::string& bgCollStr, Layer::Type type ) 
  : id( newEntityId() ) ,
  bgStr( bgStr ),
  bgCollStr( bgCollStr ),
  type( type ) 
{
  // Determine the widest row.
  unsigned maxLineWidth{};
  bool continueLooking{ true };
  LineLimits currLineLims{};
  while ( continueLooking ) {
    currLineLims.len = bgStr.find( "\n", currLineLims.start, 1 ) - currLineLims.start;
    // Stop looking when no newlines remain. Width of last line is from cursor to end of the string.
    if ( currLineLims.len < 0 ) {
      currLineLims.len = bgStr.size() - currLineLims.start;
      continueLooking = false;
    }
    // Add to line limits vector and prepare for next one.
    lineLimits.push_back( currLineLims );
    maxLineWidth = std::max( maxLineWidth, static_cast<unsigned>( currLineLims.len ) );
    currLineLims.start += currLineLims.len + 1;  // "+1" includes the newline character.
  }
  // Even though we're not right-padding the shorter lines with spaces, we'll treat
  // the grid as a rectangle for easier collision detection.
  World::set<Rect>( id, { { 0, 0 }, { static_cast<unsigned>( lineLimits.size() ), maxLineWidth } } );
}


void Grid::addLayer( const Layer& layer ) {
  _layers.push_back( layer );
}

void Grid::addEntity( const Entity entity, const unsigned layerIdx ) {
  _layers.at( layerIdx ).fg.push_back( entity );
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
  std::optional<unsigned> layerIdx{};
  const auto& pair = _entityToLayerMap.find( entity );
  if ( pair != _entityToLayerMap.end() ) {
    layerIdx = pair->second;
  }
  return layerIdx;
}


Scene::Scene( const Grid& grid ) : Window( 0, 0, COLS, LINES ), _grid(grid) {
  _camera.setDims( LINES, COLS );
}

Scene::Scene( const int x, const int y, const int w, const int h ) : Window( x, y, w, h ) {
  _camera.setDims( h, w );
}

void Scene::renderStaticLayer( const Layer& layer, const Rect& camRect ) {
  const auto& layerRect = World::get<Rect>( layer.id );
  // Background
  auto croppedRect = camRect.crop( layerRect );
  auto layerRowIdx = std::max( 0, camRect.pos.y - layerRect.pos.y );
  for ( unsigned croppedRectRowIdx{}; croppedRectRowIdx < croppedRect.size.h; ++croppedRectRowIdx, ++layerRowIdx ) {
    const auto& lineLims = layer.lineLimits.at( layerRowIdx );
    int stringStartIdx = lineLims.start;
    int stringLength = std::min( static_cast<int>( croppedRect.size.w ), lineLims.len );
    auto rowStr = std::string( layer.bgStr, stringStartIdx, stringLength );
    mvprint( croppedRect.pos.y + croppedRectRowIdx, croppedRect.pos.x, rowStr );  // layerRow + 1 to skip the stop border 
  }
  // Foreground
  for ( const auto& entity : layer.fg ) {
    if ( _camera.canSee( entity ) ) {  // TODO let camera track what it sees via onCollision(), not Scene
      const auto& entityRect = World::get<Rect>( entity );
      const auto& entityImg = World::get<Image>( entity );
      auto posWrtCamera = entityRect.pos + layerRect.pos - camRect.pos;
      setAttr ( COLOR_PAIR( entityImg.getColor() ) );
      mvprint( posWrtCamera.y, posWrtCamera.x, entityImg.getSymbol() );
      unsetAttr ( COLOR_PAIR( entityImg.getColor() ) );
    }
  }
}

void Scene::render() {
  // Camera variables
  const auto& camRect = World::get<Rect>( _camera.getId() );
  // For each layer..
  for ( const auto& layer : _grid.getLayers() ) {
    // Background
    switch ( layer.type ) {
      case Layer::Type::FIXED:     // Doesn't move with camera
        // TODO
        break;
      case Layer::Type::UNIFORM:   // Moves same velocity as camera
        renderStaticLayer( layer, camRect );
        break;
      case Layer::Type::PARALLAX:  // Moves parallax to camera's focused-on layer
        // TODO
        break;
      case Layer::Type::AUTOLOOP:  // Moves in constant velocity and loops back
        // TODO
        break;
    }
  }
}

void Scene::onInput( const InputState& input ) {
  Trigger::onTrigger( _focus, input );
}

void Scene::setFocus( Entity entity ) {
  auto layerIdx = _grid.getLayer( entity ); 
  if ( layerIdx.has_value() ) {
    _focus = entity;
    _grid.setFocusedLayerIdx( *layerIdx );
  }
}
