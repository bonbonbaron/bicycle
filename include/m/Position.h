#pragma once
#include "fixed.h"

struct Position {
  Position() = default;
  // see if automatically upshifting it from here works.
  Position( int x, int y, int z = 0) : x(x << FIXEDPT_DEC_BITS), y(y << FIXEDPT_DEC_BITS), z(z << FIXEDPT_DEC_BITS) {}
  Position( const Position& ) = default;
  Position( Position&& ) = default;
  Position& operator=( const Position& ) = default;
  Position& operator=( Position&& ) = default;
  int x{};
  int y{};
  int z{};

  void operator+=( const Position& rhs ) {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
  }

  auto operator+( const Position& rhs ) const -> Position {
    Position pos;
    pos.x = x + rhs.x;
    pos.y = y + rhs.y;
    pos.z = z + rhs.z;
    return pos;
  }

  void operator-=( const Position& rhs ) {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
  }

  auto operator-( const Position& rhs ) const -> Position {
    Position pos;
    pos.x = x - rhs.x;
    pos.y = y - rhs.y;
    pos.z = z - rhs.z;
    return pos;
  }

  auto operator>>( const unsigned n ) const -> Position {
    Position pos;
    pos.x = x >> n;
    pos.y = y >> n;
    pos.z = z >> n;
    return pos;
  }

  auto operator<<( const unsigned n ) const -> Position {
    Position pos;
    pos.x = x << n;
    pos.y = y << n;
    pos.z = z << n;
    return pos;
  }
};

