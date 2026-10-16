#include "v/common/Bar.h"
#include <algorithm>

Bar::Bar( const std::string& label, const int max, const int len, const int x, const int y ) : _x( x ), _y( y ) {
  _spec.label = label;
  _spec.max = std::max( 1, max );
  _spec.curr = _spec.max;
  _spec.len = std::max( 1, len );
}

void Bar::setValue( const int val ) {
  _spec.curr = std::clamp( val, 0, _spec.max );
}

auto Bar::getValue() const -> int {
  return _spec.curr;
}

void Bar::setPos( const int x, const int y ) {
  _x = x;
  _y = y;
}

auto Bar::filledLength() const -> int {
  return ( _spec.curr * _spec.len + _spec.max - 1 ) / _spec.max;
}

void Bar::setSprites( std::shared_ptr<const BImg> frame, std::shared_ptr<const BImg> fill ) {
  _spec.frameSprite = std::move( frame );
  _spec.fillSprite = std::move( fill );
}

void Bar::draw( const Window& tgt ) const {
  gfx::renderer().drawBar( tgt.getHandle(), _y, _x, _spec );
}
