#include "m/Camera.h"
#include "v/Window.h"
#include <algorithm>
#include "m/World.h"


/* TODO: consider using these functions:
 * int add_wchstr(const cchar_t *wchstr);
 * int add_wchnstr(const cchar_t *wchstr, int n);
 * int wadd_wchstr(WINDOW * win, const cchar_t *wchstr);
 * int wadd_wchnstr(WINDOW * win, const cchar_t *wchstr, int n);
 *
 * int mvadd_wchstr(int y, int x, const cchar_t *wchstr);
 * int mvadd_wchnstr(int y, int x, const cchar_t *wchstr, int n);
 * int mvwadd_wchstr(WINDOW *win, int y, int x, const cchar_t *wchstr);
 * int mvwadd_wchnstr(WINDOW *win, int y, int x, const cchar_t *wchstr, int n);
 *
 *
 * These functions copy the (null-terminated) array of complex characters wchstr into the window image structure starting at the current cursor position.  The four  functions
 * with  n  as  the last argument copy at most n elements, but no more than will fit on the line.  If n=-1 then the whole array is copied, to the maximum number of characters
 * that will fit on the line.
 *
 * The window cursor is not advanced.  These functions work faster than waddnstr.  On the other hand:
 *
 * •   they do not perform checking (such as for the newline, backspace, or carriage return characters),
 *
 * •   they do not advance the current cursor position,
 *
 * •   they do not expand other control characters to ^-escapes, and
 *
 * •   they truncate the string if it crosses the right margin, rather than wrapping it around to the new line.
 *
 * These functions end successfully on encountering a null cchar_t, or when they have filled the current line.  If a complex character cannot completely fit at the end of the
 * current line, the remaining columns are filled with the background character and rendition.
 */
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

// Pans just enough to keep the whole focused-on entity at least _margin inside the field of view
// (within the limits set by setLims()).
void Camera::followFocus() {
  const auto& focusBox = World::get<Box>( getFocus() );
  const int left = focusBox.pos.x;
  const int right = focusBox.pos.x + static_cast<int>( focusBox.size.w );
  const int top = focusBox.pos.y;
  const int bottom = focusBox.pos.y + static_cast<int>( focusBox.size.h );
  if ( left < getLxMargin() ) {
    pan( 0, left - getLxMargin() );
  }
  else if ( right > getHxMargin() ) {
    pan( 0, right - getHxMargin() );
  }
  if ( top < getLyMargin() ) {
    pan( top - getLyMargin(), 0 );
  }
  else if ( bottom > getHyMargin() ) {
    pan( bottom - getHyMargin(), 0 );
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
  const auto& entityImg = World::get<Image>( entity );
  // Animated entities draw only their current frame: the src rect (in character cells) of their image.
  // A zero-width rect means "draw the whole image".
  const auto& srcRect = World::get<Rect>( entity );
  const bool hasSrcRect = srcRect.w > 0;
  auto croppedBox = camBox.crop( entityBox ) >> FIXEDPT_DEC_BITS;  // portion of entityBox in FOV
  auto entityPosWrtCamera = ( entityBox.pos - camBox.pos ) >> FIXEDPT_DEC_BITS;
  // If the image's top row is out of FOV, then get the first row to draw.
  // imgRowIdx is the row of the image itself we're drawing.
  unsigned imgRowIdx = std::abs( std::min( 0, entityPosWrtCamera.y ) ) + ( hasSrcRect ? srcRect.y : 0 );
  const int clipLeft = std::abs( std::min( entityPosWrtCamera.x, 0 ) );
  for ( unsigned croppedBoxRowIdx{}; croppedBoxRowIdx < croppedBox.size.h; ++croppedBoxRowIdx, ++imgRowIdx ) {
    if ( imgRowIdx >= entityImg.lineLimits.size() ) {
      break;
    }
    const auto& lineLims = entityImg.lineLimits.at( imgRowIdx );
    int rowStart = hasSrcRect ? srcRect.x : 0;
    int rowLen = hasSrcRect ? std::min( srcRect.w, lineLims.len - srcRect.x ) : lineLims.len;
    int stringLength = std::min( static_cast<int>( croppedBox.size.w ), rowLen - clipLeft );
    if ( stringLength <= 0 ) {
      continue;
    }
    auto rowStr = std::string( entityImg.string, lineLims.start + rowStart + clipLeft, stringLength );
    // croppedBox is in world cells; the window is in screen cells.
    tgt.mvprint( croppedBox.pos.y - ( camBox.pos.y >> FIXEDPT_DEC_BITS ) + croppedBoxRowIdx,
                 croppedBox.pos.x - ( camBox.pos.x >> FIXEDPT_DEC_BITS ), rowStr );  // layerRow + 1 to skip the stop border 
  }
}
