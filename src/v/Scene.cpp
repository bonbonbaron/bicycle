#include "v/Scene.h"
#include "m/World.h"
#include <cursesw.h>
#include <algorithm>

Scene::Scene( const Grid& grid ) : Window( 0, 0, COLS, LINES ), _grid(grid) {
  _camera.setDims( LINES, COLS );
}
Scene::Scene( const int x, const int y, const int w, const int h ) : Window( x, y, w, h ) {
  _camera.setDims( h, w );
}

// This makes this class hard to classify: This is DEFINITELY the "view" portion.
// May be a good idea later to split the GUI-specific mechanisms out to another class.
void Scene::render() {
  // For each layer..
  for ( const auto& layer : _grid.getLayers() ) {
    // Background
    const auto& layerPos = World::get<Position>( layer.id );
    const auto& layerSize = World::get<Size>( layer.id );
    const auto& camPos = World::get<Position>( _camera.getId() );
    const auto& camSize = World::get<Size>( _camera.getId() );
    const unsigned MAX_WIDTH{ getWidth() - WINDOW_PADDING };
    unsigned croppedWidth{}; 
    // If grid starts to the left of FOV...
    if ( layerPos.x < camPos.x ) {
      croppedWidth = std::min( MAX_WIDTH, ( layerSize.w + layerPos.x - camPos.x ) );
    }
    // Else if grid ends to the right of FOV...
    else if ( ( layerPos.x + layerSize.w ) > ( camPos.x + MAX_WIDTH + 1 ) ) {
      croppedWidth = std::min( MAX_WIDTH, ( camPos.x + camSize.w - layerPos.x ) );
    }
    else {
      croppedWidth = std::min( MAX_WIDTH, layerSize.w );
    }
    auto lastRow = std::min<unsigned>( getHeight() - WINDOW_PADDING, layerPos.y + layerSize.h );
    for ( unsigned row = layerPos.y; row <= lastRow; ++row ) {
      int startPos =  ( camPos.y + row ) * layerSize.w + camPos.x;
      int stringLength = croppedWidth;
      auto rowStr = std::string( layer.bgStr, startPos, stringLength );
      mvprint( row + 1, 1, rowStr );  // row + 1 to skip the stop border 
    }
    // Foreground
    for ( const auto& entity : layer.fg ) {
      if ( _camera.canSee( entity ) ) {  // TODO let camera track what it sees via onCollision(), not Scene
        const auto& pos = World::get<Position>( entity );
        const auto& img = World::get<Image>( entity );
        auto xRelToCamera = pos.x - camPos.x;
        auto yRelToCamera = pos.y - camPos.y;
        setAttr ( COLOR_PAIR( img.getColor() ) );
        mvprint( yRelToCamera, xRelToCamera, img.getSymbol() );
        unsetAttr ( COLOR_PAIR( img.getColor() ) );
      }
    }
  }
}

void Scene::onInput( const InputState& input ) {
  // auto focus = _camera.getFocus();  // TODO
  // focus->onInput( input );  TODO entities no longer have built-in functions
}

