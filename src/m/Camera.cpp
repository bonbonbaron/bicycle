#include "m/Camera.h"
#include "v/common/Window.h"
#include <algorithm>
#include "m/World.h"
#include "v/common/Renderer.h"
#include "c/Kinematics.h"


Camera::Camera() : _id( newEntityId() ) {}

Camera::Camera( const int x, const int y, const int margin ) : _id( newEntityId() ), _margin(margin) {
  World::set<Box>( _id, { { x, y, 0 }, { static_cast<unsigned>(gfx::cols() << FIXEDPT_DEC_BITS), static_cast<unsigned>(gfx::lines() << FIXEDPT_DEC_BITS), 0 } } );
}

void Camera::pan( const int dy, const int dx ) {
  auto& box = World::get<Box>( _id );
  box.pos.x += dx;
  box.pos.y += dy;
  box.pos.x = std::clamp<int>( box.pos.x, 0, _maxX );
  box.pos.y = std::clamp<int>( box.pos.y, 0, _maxY );
}

// TODO this assumes single-character symbols. Handle multi-char later.
auto Camera::canSee( const Entity entity ) const -> bool {
  const auto& entityBox = World::get<Box>( entity );  // entity->body.getPosition();
  const auto& camBox = World::get<Box>( _id );  // entity->body.getPosition();
  return _canSee( camBox, entityBox );
}

// The camera doesn't move itself: it's a kinematic body tracking its focus through Kinematics' FOLLOW_LERP, like
// any follower. The default is the old behavior: hard tracking with a dead zone that keeps the focus _margin cells
// inside the view. Lua picks the mode (Camera.track in scripts/bicycle.lua: mode = "hard" or "lerp", rate,
// offset, deadzone) and can switch at run time.
void Camera::focusOn( const Entity entity ) {
  _focus = entity;
  const auto& cam = World::get<Box>( _id );
  const auto& tgt = World::get<Box>( entity );
  constexpr float ONE = 1 << FIXEDPT_DEC_BITS;
  const float m = _margin / ONE;
  const float deadX = std::max( 0.f, ( cam.size.w - tgt.size.w ) / ( 2 * ONE ) - m );
  const float deadY = std::max( 0.f, ( cam.size.h - tgt.size.h ) / ( 2 * ONE ) - m );
  Kinematics::getInstance().follow( _id, entity, 0, 0, 0, 0, 0, false, deadX, deadY );
}

auto Camera::getFocus() const -> Entity {
  return _focus;
}

void Camera::setDims( const unsigned h, const unsigned w) {
  auto& box = World::get<Box>( _id );
  box.size = Size(w, h);
}

// Highest camera position (fixed point) in each dimension; the lowest is 0.
void Camera::setLims( const int y, const int x) {
  _maxY = y;
  _maxX = x;
}

// margin is in cells.
void Camera::setMargin( const int margin ) {
  _margin = margin << FIXEDPT_DEC_BITS;
}

auto Camera::getLxMargin() const -> int {
  const auto& camBox = World::get<Box>( _id );  // entity->body.getPosition();
  return camBox.pos.x + _margin;
}

auto Camera::getHxMargin() const -> int {
  const auto& camBox = World::get<Box>( _id );  // entity->body.getPosition();
  return camBox.pos.x + static_cast<int>( camBox.size.w ) - _margin;
}

auto Camera::getLyMargin() const -> int {
  const auto& camBox = World::get<Box>( _id );  // entity->body.getPosition();
  return camBox.pos.y + _margin;
}

auto Camera::getHyMargin() const -> int {
  const auto& camBox = World::get<Box>( _id );  // entity->body.getPosition();
  return camBox.pos.y + static_cast<int>( camBox.size.h ) - _margin;
}

auto Camera::getId() const -> Entity {
  return _id;
}

// Images don't own boxs because motion system needs those to be separate.
// So instead we access those via entity ID.
// TODO distinguish between drawing backgrounds and foregrounds
void Camera::draw( const Entity entity, Window& tgt ) {
  // TODO Consider making a pointer to this.
  const auto& camBox = World::get<Box>( _id );  // entity->body.getPosition();
  //TODO if box and image sizes don't match, imge should scale to box
  const auto& entityBox = World::get<Box>( entity );  
  if ( ! camBox.overlaps( entityBox ) ) {  // TODO let camera track what it sees via onCollision(), not Scene
    return;
  }
  gfx::renderer().drawEntity( tgt.getHandle(), entity, camBox );
}
