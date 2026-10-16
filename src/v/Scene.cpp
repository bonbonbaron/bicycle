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
  _camera.setDims( getHeight(), getWidth() );
  _camera.setMargin( 10 );  // TODO set margin in a bicycle API call
}

Scene::Scene( const int x, const int y, const int w, const int h ) : Window( x, y, w, h ) {
  _camera.setDims( getHeight(), getWidth() );
  _camera.setMargin( 10 );  // TODO set margin in a bicycle API call
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
  _camera.setLims( std::max( 0, worldH - static_cast<int>( camBox.size.h ) ),
                   std::max( 0, worldW - static_cast<int>( camBox.size.w ) ) );
}

static auto wrapIdx( const int i, const int n ) -> int {
  return ( ( i % n ) + n ) % n;
}

// Draws a background layer so that screen cell (r, c) shows image cell (r + offY, c + offX), wrapping
// around the image if wrap is set. Spaces are transparent so layers behind show through.
void Scene::renderBg( const Layer& layer, const int offX, const int offY, const bool wrap ) {
  const auto& img = World::get<Image>( layer.id );
  const int imgH = img.lineLimits.size();
  const int imgW = img.size.w >> FIXEDPT_DEC_BITS;
  if ( imgH == 0 || imgW == 0 ) {
    return;
  }
  const int h = getHeight();
  const int w = getWidth();
  std::string row( w, ' ' );
  for ( int r = 0; r < h; ++r ) {
    int iy = r + offY;
    if ( wrap ) {
      iy = wrapIdx( iy, imgH );
    }
    else if ( iy < 0 || iy >= imgH ) {
      continue;
    }
    const auto& lims = img.lineLimits.at( iy );
    for ( int c = 0; c < w; ++c ) {
      int ix = c + offX;
      if ( wrap ) {
        ix = wrapIdx( ix, imgW );
      }
      row[c] = ( ix >= 0 && ix < lims.len ) ? img.string[ lims.start + ix ] : ' ';
    }
    // Print runs of non-space characters only.
    for ( int c = 0; c < w; ) {
      if ( row[c] == ' ' ) {
        ++c;
        continue;
      }
      int end = c;
      while ( end < w && row[end] != ' ' ) {
        ++end;
      }
      mvprint( r, c, row.substr( c, end - c ) );
      c = end;
    }
  }
}

void Scene::renderFg( const Layer& layer ) {
  for ( const auto& entity : layer.fg ) {
    if ( auto it = _hidden.find( entity ); it != _hidden.end() && it->second ) {
      continue;
    }
    _camera.draw( entity, *this );
  }
}

void Scene::render() {
  updateCameraLimits();
  _camera.followFocus();
  const auto& camPos = World::get<Box>( _camera.getId() ).pos;
  const int camX = camPos.x >> FIXEDPT_DEC_BITS;
  const int camY = camPos.y >> FIXEDPT_DEC_BITS;
  for ( auto& layer : _grid.getLayers() ) {
    const auto& pos = World::get<Box>( layer.id ).pos;
    const int x = pos.x >> FIXEDPT_DEC_BITS;
    const int y = pos.y >> FIXEDPT_DEC_BITS;
    const int px = camX * layer.parallaxPct / 100;
    const int py = camY * layer.parallaxPctY / 100;
    switch ( layer.type ) {
      case LayerType::FIXED:     // Part of the world: the camera scrolls over it
        renderBg( layer, camX - x, camY - y, false );
        break;
      case LayerType::GLUED:     // Moves with the camera, i.e. stays put on screen (skies, HUD art)
        renderBg( layer, -x, -y, false );
        break;
      case LayerType::PARALLAX:  // Scrolls at parallaxPct of the camera's speed
        renderBg( layer, px - x, py - y, false );
        break;
      case LayerType::LOOP:      // Like PARALLAX, but repeats endlessly past its edges
        renderBg( layer, px - x, py - y, true );
        break;
      case LayerType::AUTOLOOP:  // Drifts at a constant velocity and repeats endlessly
        layer.drift += layer.vel;
        renderBg( layer, px - x + ( layer.drift.x >> FIXEDPT_DEC_BITS ), py - y + ( layer.drift.y >> FIXEDPT_DEC_BITS ), true );
        break;
    }
    renderFg( layer );
  }
  renderHud();
  renderTextBox();
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

void Scene::setTextBox( const std::vector<std::string>& lines ) {
  _textBox = lines;
}

// The box spans the window inside its border with one column of padding: | text |
auto Scene::getTextBoxWidth() const -> int {
  return std::max( 1, static_cast<int>( getWidth() ) - 6 );
}

void Scene::renderTextBox() {
  if ( _textBox.empty() ) {
    return;
  }
  const int w = getTextBoxWidth() + 4;           // + "| " and " |"
  const int h = static_cast<int>( _textBox.size() ) + 2;
  const int top = std::max( 1, static_cast<int>( getHeight() ) - 1 - h );  // just above the bottom border
  const std::string edge = "+" + std::string( w - 2, '-' ) + "+";
  mvprint( top, 1, edge );
  for ( int i = 0; i < static_cast<int>( _textBox.size() ); ++i ) {
    std::string row = _textBox.at( i ).substr( 0, w - 4 );
    row.resize( w - 4, ' ' );
    mvprint( top + 1 + i, 1, "| " + row + " |" );
  }
  mvprint( top + h - 1, 1, edge );
}

void Scene::onInput( Input& input ) {
}

// A full-screen scene grows or shrinks with the terminal, e.g. when a terminal is maximized after the game started.
void Scene::onTermResize() {
  setWidth( COLS - 4 );  // same size the default constructor gives
  setHeight( LINES );
  _camera.setDims( getHeight(), getWidth() );
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
