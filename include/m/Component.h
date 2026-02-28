#pragma once

struct Position {
  int x{};
  int y{};
};

struct Size {
  unsigned w{};
  unsigned h{};
};

struct Image {
  Position pos{};
  Size size{};
  Color color{};
};

// TODO make a singleton structure of arrays... if there's a more efficient pattern than singleton pattern for this, lmk
