#pragma once
#include "m/Position.h"
#include "m/Size.h"

static constexpr decltype(Position::x) FIXEDPT_DEC_BITS{8};

struct Rect {
  Rect() = default;
  //World::set<Rect>( id, 
      //{ /* pos */ { 0, 0, 0 }, /* size */ { static_cast<unsigned>( lineLimits.size() ), bgImg.size, 0 } } );
  Rect( const Position& pos, const Size& size ) : pos(pos), size(size) {}
  Rect( const Position&& pos, const Size&& size ) : pos(pos), size(size) {}
  Rect( const Rect& ) = default;
  Rect( Rect&& ) = default;

  Rect& operator=( const Rect& ) = default;
  Rect& operator=( Rect&& ) = default;

  Position pos{};
  Size size{};

  auto overlaps( const Rect& rhs ) const -> bool;
  auto crop( const Rect& rhs ) const -> Rect;

  void operator+=( const Rect& rhs ) {
    pos.x += rhs.pos.x;
    pos.y += rhs.pos.y;
    pos.z += rhs.pos.z;
    size.w += rhs.size.w;
    size.h += rhs.size.h;
    size.d += rhs.size.d;
  }

  auto operator+( const Rect& rhs ) const -> Rect {
    Rect output;
    output.pos.x = pos.x + rhs.pos.x;
    output.pos.y = pos.y + rhs.pos.y;
    output.pos.z = pos.z + rhs.pos.z;
    output.size.w = size.w + rhs.size.w;
    output.size.h = size.h + rhs.size.h;
    output.size.d = size.d + rhs.size.d;
    return output;
  }

  void operator-=( const Rect& rhs ) {
    pos.x -= rhs.pos.x;
    pos.y -= rhs.pos.y;
    pos.z -= rhs.pos.z;
    size.w -= rhs.size.w;
    size.h -= rhs.size.h;
    size.d -= rhs.size.d;
  }

  auto operator-( const Rect& rhs ) const -> Rect {
    Rect output;
    output.pos.x = pos.x - rhs.pos.x;
    output.pos.y = pos.y - rhs.pos.y;
    output.pos.z = pos.z - rhs.pos.z;
    output.size.w = size.w - rhs.size.w;
    output.size.h = size.h - rhs.size.h;
    output.size.d = size.d - rhs.size.d;
    return output;
  }

  auto operator>>( const unsigned n ) const -> Rect {
    Rect output;
    output.pos.x = pos.x >> n;
    output.pos.y = pos.y >> n;
    output.pos.z = pos.z >> n;
    output.size.w = size.w >> n;
    output.size.h = size.h >> n;
    output.size.d = size.d >> n;
    return output;
  }

};

