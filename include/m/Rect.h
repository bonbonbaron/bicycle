#pragma once
#include "m/Position.h"
#include "m/Size.h"

struct Rect {
  Rect() = default;
  Rect( const Position& pos, const Size& size ) : pos(pos), size(size) {}
  Rect( const Rect& ) = default;
  Rect( Rect&& ) = default;

  Rect& operator=( const Rect& ) = default;
  Rect& operator=( Rect&& ) = default;

  Position pos{};
  Size size{};

  auto overlaps( const Rect& rhs ) const -> bool;
  auto crop( const Rect& rhs ) const -> Rect;
};

