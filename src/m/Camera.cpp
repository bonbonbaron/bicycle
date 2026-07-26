#include "m/Camera.h"
#include "v/Window.h"
#include <algorithm>
#include "m/World.h"

Camera::Camera() : _id( newEntityId() ) {}

Camera::Camera( const int x, const int y, const int margin ) : _id( newEntityId() ), _margin(margin) {
  World::set<Box>( _id, { { x, y, 0 }, { static_cast<unsigned>(COLS << FIXEDPT_DEC_BITS), static_cast<unsigned>(LINES << FIXEDPT_DEC_BITS), 0 } } );
}

void Camera::pan( const int dy, const int dx ) {
  auto& box = World::get<Box>( _id );
  box.pos.x += dx;
  box.pos.y += dy;
  box.pos.x = std::clamp<int>( box.pos.x, 0, _maxX );
  box.pos.y = std::clamp<int>( box.pos.y, 0, _maxY );
}

void Camera::followFocus() {
  auto& focusBox = World::get<Box>( getFocus() );
  auto lx = getLxMargin();
  auto hx = getHxMargin();
  auto ly = getLyMargin();
  auto hy = getHyMargin();
  if ( focusBox.pos.x <= lx ) {
    pan( 0, focusBox.pos.x - lx );
  }
  else if ( focusBox.pos.x >= hx ) {
    pan( 0, focusBox.pos.x - hx );
  }
  if ( focusBox.pos.y <= ly ) {
    pan( focusBox.pos.y - ly, 0 );
  }
  else if ( focusBox.pos.y >= hy ) {
    pan( focusBox.pos.y - hy, 0 );
  }
}

// TODO this assumes single-character symbols. Handle multi-char later.
auto Camera::canSee( const Entity entity ) const -> bool {
  const auto& entityBox = World::get<Box>( entity );  // entity->body.getPosition();
  const auto& camBox = World::get<Box>( _id );  // entity->body.getPosition();
  return _canSee( camBox, entityBox );
}

void Camera::focusOn( const Entity entity ) {
  _focus = entity;
}

auto Camera::getFocus() const -> Entity {
  return _focus;
}

void Camera::setDims( const unsigned h, const unsigned w) {
  auto& box = World::get<Box>( _id );
  box.size = Size(w, h);
}

void Camera::setLims( const int y, const int x) {
  _maxY = y;
  _maxX = x;
}

void Camera::setMargin( const int margin ) {
  _margin = margin;
}

auto Camera::getLxMargin() const -> int {
  const auto& camBox = World::get<Box>( _id );  // entity->body.getPosition();
  return camBox.pos.x + _margin;
}

auto Camera::getHxMargin() const -> int {
  const auto& camBox = World::get<Box>( _id );  // entity->body.getPosition();
  return camBox.pos.x + camBox.size.w - _margin + (WINDOW_PADDING/2);
}

auto Camera::getLyMargin() const -> int {
  const auto& camBox = World::get<Box>( _id );  // entity->body.getPosition();
  return camBox.pos.y + _margin;
}

auto Camera::getHyMargin() const -> int {
  const auto& camBox = World::get<Box>( _id );  // entity->body.getPosition();
  return camBox.pos.y + camBox.size.h - _margin + (WINDOW_PADDING/2);
}

auto Camera::getId() const -> Entity {
  return _id;
}

// Images don't own boxs because motion system needs those to be separate.
// So instead we access those via entity ID.
void Camera::draw( const Entity entity, Window& tgt ) {
  // TODO Consider making a pointer to this.
  const auto& camBox = World::get<Box>( _id );  // entity->body.getPosition();
  //TODO if box and image sizes don't match, imge should scale to box
  const auto& entityBox = World::get<Box>( entity );  
  if ( ! _canSee( camBox, entityBox ) ) {  // TODO let camera track what it sees via onCollision(), not Scene
    return;
  }
  const auto& entityImg = World::get<Image>( entity );
  auto croppedBox = camBox.crop( entityBox ) >> FIXEDPT_DEC_BITS;  // portion of entityBox in FOV
  auto entityPosWrtCamera = ( entityBox.pos - camBox.pos ) >> FIXEDPT_DEC_BITS;
  // If the image's top row is out of FOV, then get the first row to draw.
  // imgRowIdx is the row of the image itself we're drawing.
  auto imgRowIdx = std::abs( std::min( 0, entityPosWrtCamera.y ) );  
  for ( unsigned croppedBoxRowIdx{}; croppedBoxRowIdx < croppedBox.size.h; ++croppedBoxRowIdx, ++imgRowIdx ) {
    const auto& lineLims = entityImg.lineLimits.at( imgRowIdx );
    int stringStartIdx = lineLims.start 
      + std::abs( std::min( entityPosWrtCamera.x, 0 ) );
    int stringLength = std::min( static_cast<int>( croppedBox.size.w ), lineLims.len );
    auto rowStr = std::string( entityImg.string, stringStartIdx, stringLength );
    tgt.mvprint( croppedBox.pos.y + croppedBoxRowIdx, croppedBox.pos.x, rowStr );  // layerRow + 1 to skip the stop border 
  }
}
