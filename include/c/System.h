#pragma once
#include "m/World.h"

class System {
  public:
    System() = default;

    // Virtual member functions
    virtual void run() = 0;

};
