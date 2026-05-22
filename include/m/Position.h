#pragma once

struct Position {
  Position() = default;
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
};
