#include "m/Camera.h"
#include "v/Window.h"
#include <algorithm>
#include "m/World.h"

Camera::Camera() : _id( newEntityId() ) {}

Camera::Camera( const int x, const int y, const int margin ) : _id( newEntityId() ), _margin(margin) {
  World::set<Rect>( _id, { { x, y }, { static_cast<unsigned>(COLS), static_cast<unsigned>(LINES) } } );
}

void Camera::pan( const int dy, const int dx ) {
  auto& rect = World::get<Rect>( _id );
  rect.pos.x += dx;
  rect.pos.y += dy;
  rect.pos.x = std::clamp<int>( rect.pos.x, 0, _maxX );
  rect.pos.y = std::clamp<int>( rect.pos.y, 0, _maxY );
}

void Camera::followFocus() {
  auto& focusRect = World::get<Rect>( getFocus() );
  auto lx = getLxMargin();
  auto hx = getHxMargin();
  auto ly = getLyMargin();
  auto hy = getHyMargin();
  if ( focusRect.pos.x <= lx ) {
    pan( 0, focusRect.pos.x - lx );
  }
  else if ( focusRect.pos.x >= hx ) {
    pan( 0, focusRect.pos.x - hx );
  }
  if ( focusRect.pos.y <= ly ) {
    pan( focusRect.pos.y - ly, 0 );
  }
  else if ( focusRect.pos.y >= hy ) {
    pan( focusRect.pos.y - hy, 0 );
  }
}

// TODO this assumes single-character symbols. Handle multi-char later.
auto Camera::canSee( const Entity entity ) const -> bool {
  const auto& entityRect = World::get<Rect>( entity );  // entity->body.getPosition();
  const auto& camRect = World::get<Rect>( _id );  // entity->body.getPosition();
  return _canSee( camRect, entityRect );
}

void Camera::focusOn( const Entity entity ) {
  _focus = entity;
}

auto Camera::getFocus() const -> Entity {
  return _focus;
}

void Camera::setDims( const unsigned h, const unsigned w) {
  auto& rect = World::get<Rect>( _id );
  rect.size.w = w;
  rect.size.h = h;
}

void Camera::setLims( const int y, const int x) {
  _maxY = y;
  _maxX = x;
}

auto Camera::getLxMargin() const -> int {
  const auto& camRect = World::get<Rect>( _id );  // entity->body.getPosition();
  return camRect.pos.x + _margin;
}

auto Camera::getHxMargin() const -> int {
  const auto& camRect = World::get<Rect>( _id );  // entity->body.getPosition();
  return camRect.pos.x + camRect.size.w - _margin + (WINDOW_PADDING/2);
}

auto Camera::getLyMargin() const -> int {
  const auto& camRect = World::get<Rect>( _id );  // entity->body.getPosition();
  return camRect.pos.y + _margin;
}

auto Camera::getHyMargin() const -> int {
  const auto& camRect = World::get<Rect>( _id );  // entity->body.getPosition();
  return camRect.pos.y + camRect.size.h - _margin + (WINDOW_PADDING/2);
}

auto Camera::getId() const -> Entity {
  return _id;
}

// Images don't own rects because motion system needs those to be separate.
// So instead we access those via entity ID.
void Camera::draw( const Entity entity, Window& tgt ) {
  // TODO Consider making a pointer to this.
  const auto& camRect = World::get<Rect>( _id );  // entity->body.getPosition();
  //TODO if rect and image sizes don't match, imge should scale to rect
  const auto& entityRect = World::get<Rect>( entity );  
  if ( ! _canSee( camRect, entityRect ) ) {  // TODO let camera track what it sees via onCollision(), not Scene
    return;
  }
  const auto& entityImg = World::get<Image>( entity );
  auto croppedRect = camRect.crop( entityRect ) >> FIXEDPT_DEC_BITS;  // portion of entityRect in FOV
  auto entityPosWrtCamera = ( entityRect.pos - camRect.pos ) >> FIXEDPT_DEC_BITS;
  // If the image's top row is out of FOV, then get the first row to draw.
  // imgRowIdx is the row of the image itself we're drawing.
  auto imgRowIdx = std::abs( std::min( 0, entityPosWrtCamera.y ) );  
  for ( unsigned croppedRectRowIdx{}; croppedRectRowIdx < croppedRect.size.h; ++croppedRectRowIdx, ++imgRowIdx ) {
    const auto& lineLims = entityImg.lineLimits.at( imgRowIdx );
    int stringStartIdx = lineLims.start 
      + std::abs( std::min( entityPosWrtCamera.x, 0 ) );
    int stringLength = std::min( static_cast<int>( croppedRect.size.w ), lineLims.len );
    auto rowStr = std::string( entityImg.string, stringStartIdx, stringLength );
    tgt.mvprint( croppedRect.pos.y + croppedRectRowIdx, croppedRect.pos.x, rowStr );  // layerRow + 1 to skip the stop border 
  }
}
