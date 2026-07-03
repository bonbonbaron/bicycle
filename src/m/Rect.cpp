#include "m/Rect.h"
#include <algorithm>

auto Rect::overlaps( const Rect& rhs ) const -> bool {
  return this->pos.y                             < rhs.pos.y + static_cast<int>(rhs.size.h) &&  
    this->pos.y + static_cast<int>(this->size.h) > rhs.pos.y                                && 
    this->pos.x                                  < rhs.pos.x + static_cast<int>(rhs.size.w) &&
    this->pos.x + static_cast<int>(this->size.w) > rhs.pos.x;        
}

auto Rect::crop( const Rect& rhs ) const -> Rect {
  Rect output{};
  if ( this->overlaps( rhs ) ) {
    // Position of cropped rectangle
    output.pos.x = std::max( this->pos.x, rhs.pos.x );
    output.pos.y = std::max( this->pos.y, rhs.pos.y );
    // Size of cropped rectangle ( min right size - max left side )
    output.size.w =   std::min( ( rhs.pos.x + rhs.size.w ), ( this->pos.x + this->size.w ) )
                    - std::max( rhs.pos.x, this->pos.x );
    output.size.h =   std::min( ( rhs.pos.y + rhs.size.h ), ( this->pos.y + this->size.h ) )
                    - std::max( rhs.pos.y, this->pos.y );
  }
  return output;
}
