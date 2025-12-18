#pragma once
#include "Contexth"

template <typename T>
class FloatingCursor : public Context {
  public:
    FloatingCursor() = default;
    FloatingCursor(const int x, const int y);
    void react( const int input ) override;
    void update() override;
    void show() override;
    void hide() override;
    void move( const int x, const int y ) const;
  private:
    int _x{}, _y{};
};
