#include "m/Camera.h"
#include "v/common/Window.h"
#include <algorithm>
#include "m/World.h"
#include "v/common/Renderer.h"


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

// Pans just enough to keep the whole focused-on entity at least _margin inside the field of view
// (within the limits set by setLims()). Each axis is followed on its own:
// * If the view is too small for both margins plus the entity, the margin shrinks to fit, and an entity
//   taller (or wider) than the view is centered. Otherwise the low and high margins can't both be met,
//   and the camera would pan to one, then back to the other, every frame (a shaking screen).
// * The camera only ever moves toward the side the entity crossed, so it never undoes its own pan.
static auto followAxis( const int camPos, const int camLen, const int margin, const int lo, const int hi ) -> int {
  const int entityLen = hi - lo;
  if ( entityLen >= camLen ) {
    return lo + entityLen / 2 - camLen / 2;  // too big to fit: center it
  }
  const int m = std::min( margin, ( camLen - entityLen ) / 2 );
  if ( lo < camPos + m ) {
    return lo - m;
  }
  if ( hi > camPos + camLen - m ) {
    return hi - camLen + m;
  }
  return camPos;
}

void Camera::followFocus() {
  const auto& focusBox = World::get<Box>( getFocus() );
  const auto& camBox = World::get<Box>( _id );
  const int x = followAxis( camBox.pos.x, static_cast<int>( camBox.size.w ), _margin,
                            focusBox.pos.x, focusBox.pos.x + static_cast<int>( focusBox.size.w ) );
  const int y = followAxis( camBox.pos.y, static_cast<int>( camBox.size.h ), _margin,
                            focusBox.pos.y, focusBox.pos.y + static_cast<int>( focusBox.size.h ) );
  pan( y - camBox.pos.y, x - camBox.pos.x );
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
