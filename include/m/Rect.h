#pragma once

struct Rect {
  Rect() = default;
  Rect(int x, int y, int w, int h ) : x(x), y(y), w(w), h(h){}
  int x{};
  int y{};
  int w{};
  int h{};
};
