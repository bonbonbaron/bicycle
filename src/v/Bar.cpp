#include "v/Bar.h"
#include <algorithm>

Bar::Bar( const std::string& label, const int max, const int len, const int x, const int y ) :
  _label( label ),
  _curr( max ),
  _max( std::max( 1, max ) ),
  _len( std::max( 1, len ) ),
  _x( x ),
  _y( y ) {}

void Bar::setValue( const int val ) {
  _curr = std::clamp( val, 0, _max );
}

auto Bar::getValue() const -> int {
  return _curr;
}

void Bar::setPos( const int x, const int y ) {
  _x = x;
  _y = y;
}

auto Bar::filledLength() const -> int {
  return ( _curr * _len + _max - 1 ) / _max;
}

void Bar::draw( const Window& tgt ) const {
  const int filled = filledLength();
  std::string s = _label.empty() ? "[" : _label + " [";
  s += std::string( filled, '#' ) + std::string( _len - filled, '-' ) + "] ";
  s += std::to_string( _curr ) + "/" + std::to_string( _max );
  tgt.mvprint( _y, _x, s );
}
