#include "m/Camera.h"
#include "v/Window.h"
#include <algorithm>
#include "m/World.h"

Camera::Camera() : _id( newEntityId() ) {}

Camera::Camera( const int x, const int y, const int margin ) : _id( newEntityId() ), _margin(margin) {
  World::set<Position>( _id, { x, y } );
  // World::set<Size>( _id, { x, y } );  // TODO size cannot be set from this information alone.
}

void Camera::pan( const int dy, const int dx ) {
  auto& pos = World::get<Position>( _id );
  pos.x += dx;
  pos.y += dy;
  pos.x = std::clamp<int>( pos.x, 0, _maxX );
  pos.y = std::clamp<int>( pos.y, 0, _maxY );
}

void Camera::followFocus() {
  auto& fpos = World::get<Position>( getFocus() );
  auto lx = getLxMargin();
  auto hx = getHxMargin();
  auto ly = getLyMargin();
  auto hy = getHyMargin();
  if ( fpos.x <= lx ) {
    pan( 0, fpos.x - lx );
  }
  else if ( fpos.x >= hx ) {
    pan( 0, fpos.x - hx );
  }
  if ( fpos.y <= ly ) {
    pan( fpos.y - ly, 0 );
  }
  else if ( fpos.y >= hy ) {
    pan( fpos.y - hy, 0 );
  }
}

// TODO this assumes single-character symbols. Handle multi-char later.
auto Camera::canSee( const Entity entity ) const -> bool {
  const auto& entityPos = World::get<Position>( entity );  // entity->body.getPosition();
  const auto& camPos = World::get<Position>( _id );  // entity->body.getPosition();
  auto& camSize = World::get<Size>( _id );
  return 
    entityPos.x >= static_cast<decltype(Position::x)>( camPos.x ) && 
    entityPos.x <= static_cast<decltype(Position::x)>( camPos.x + camSize.w ) &&
    entityPos.y >= static_cast<decltype(Position::y)>( camPos.y ) && 
    entityPos.y <= static_cast<decltype(Position::y)>( camPos.y + camSize.h );
}

void Camera::focusOn( const Entity entity ) {
  _focus = entity;
}

auto Camera::getFocus() const -> Entity {
  return _focus;
}

void Camera::setDims( const unsigned h, const unsigned w) {
  World::set<Size>( _id, {w, h} );
}

void Camera::setLims( const int y, const int x) {
  _maxY = y;
  _maxX = x;
}

auto Camera::getLxMargin() const -> int {
  const auto& camPos = World::get<Position>( _id );  // entity->body.getPosition();
  return camPos.x + _margin;
}

auto Camera::getHxMargin() const -> int {
  const auto& camPos = World::get<Position>( _id );  // entity->body.getPosition();
  const auto& camSize = World::get<Size>( _id );
  return camPos.x + camSize.w - _margin + (WINDOW_PADDING/2);
}

auto Camera::getLyMargin() const -> int {
  const auto& camPos = World::get<Position>( _id );  // entity->body.getPosition();
  return camPos.y + _margin;
}

auto Camera::getHyMargin() const -> int {
  const auto& camPos = World::get<Position>( _id );  // entity->body.getPosition();
  const auto& camSize = World::get<Size>( _id );
  return camPos.y + camSize.h - _margin + (WINDOW_PADDING/2);
}

auto Camera::getId() const -> Entity {
  return _id;
}

// TODO write onCollide() to handle entities coming onto and off of the screen
