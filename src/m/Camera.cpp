#include "m/Camera.h"
#include "v/Window.h"
#include <algorithm>
#include "m/World.h"

Camera::Camera( const int x, const int y, const int margin ) : _x(x), _y(y), _margin(margin) {}

void Camera::pan( const int dy, const int dx ) {
  _x += dx;
  _y += dy;
  _x = std::clamp<int>( _x, 0, _maxX );
  _y = std::clamp<int>( _y, 0, _maxY );
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
  auto& pos = World::get<Position>( entity );  // entity->body.getPosition();
  return 
    pos.x >= _x && 
    pos.x <= _x + _w &&
    pos.y >= _y && 
    pos.y <= _y + _h;
}

void Camera::focusOn( const Entity entity ) {
  _focus = entity;
}

auto Camera::getFocus() const -> Entity {
  return _focus;
}

void Camera::setDims( const int h, const int w) {
  _h = h;
  _w = w;
}

void Camera::setLims( const int y, const int x) {
  _maxY = y;
  _maxX = x;
}

auto Camera::getX() const -> int {
  return _x;
}

auto Camera::getY() const -> int {
  return _y;
}

auto Camera::getLxMargin() const -> int {
  return _x + _margin;
}

auto Camera::getHxMargin() const -> int {
  return _x + _w - _margin + (WINDOW_PADDING/2);
}

auto Camera::getLyMargin() const -> int {
  return _y + _margin;
}

auto Camera::getHyMargin() const -> int {
  return _y + _h - _margin + (WINDOW_PADDING/2);
}

// TODO write onCollide() to handle entities coming onto and off of the screen
