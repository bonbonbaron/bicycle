#pragma once
#include "m/Position.h"
#include "m/Size.h"

struct Box {
  Box() = default;
  //World::set<Box>( id, 
      //{ /* pos */ { 0, 0, 0 }, /* size */ { static_cast<unsigned>( lineLimits.size() ), bgImg.size, 0 } } );
  Box( const Position& pos, const Size& size ) : pos(pos), size(size) {}
  Box( const Position&& pos, const Size&& size ) : pos(pos), size(size) {}
  Box( const Box& ) = default;
  Box( Box&& ) = default;

  Box& operator=( const Box& ) = default;
  Box& operator=( Box&& ) = default;

  Position pos{};
  Size size{};

  auto overlaps( const Box& rhs ) const -> bool;
  auto crop( const Box& rhs ) const -> Box;

  void operator+=( const Box& rhs ) {
    pos.x += rhs.pos.x;
    pos.y += rhs.pos.y;
    pos.z += rhs.pos.z;
    size.w += rhs.size.w;
    size.h += rhs.size.h;
    size.d += rhs.size.d;
  }

  auto operator+( const Box& rhs ) const -> Box {
    Box output;
    output.pos.x = pos.x + rhs.pos.x;
    output.pos.y = pos.y + rhs.pos.y;
    output.pos.z = pos.z + rhs.pos.z;
    output.size.w = size.w + rhs.size.w;
    output.size.h = size.h + rhs.size.h;
    output.size.d = size.d + rhs.size.d;
    return output;
  }

  void operator-=( const Box& rhs ) {
    pos.x -= rhs.pos.x;
    pos.y -= rhs.pos.y;
    pos.z -= rhs.pos.z;
    size.w -= rhs.size.w;
    size.h -= rhs.size.h;
    size.d -= rhs.size.d;
  }

  auto operator-( const Box& rhs ) const -> Box {
    Box output;
    output.pos.x = pos.x - rhs.pos.x;
    output.pos.y = pos.y - rhs.pos.y;
    output.pos.z = pos.z - rhs.pos.z;
    output.size.w = size.w - rhs.size.w;
    output.size.h = size.h - rhs.size.h;
    output.size.d = size.d - rhs.size.d;
    return output;
  }

  auto operator>>( const unsigned n ) const -> Box {
    Box output;
    output.pos.x = pos.x >> n;
    output.pos.y = pos.y >> n;
    output.pos.z = pos.z >> n;
    output.size.w = size.w >> n;
    output.size.h = size.h >> n;
    output.size.d = size.d >> n;
    return output;
  }

};

