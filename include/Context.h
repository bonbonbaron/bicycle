#pragma once
#include  <map>
#include <functional>


class Context {
  public:
    Context() = default;
    virtual void react( const int input ) = 0;
    virtual void update() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
};
