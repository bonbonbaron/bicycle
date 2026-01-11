#pragma once
#include "Node.h"

class Controller : public bicycle::Node {
  public:
    // We return this as a Node since it's no different besides the singleton pattern.
    static auto getInstance() -> bicycle::Node&  {
      static Controller ctlr;
      return ctlr;
    }
  private:
    Controller() = default;
    Controller( const Controller& ) = delete;
    Controller& operator=( Controller& ) = delete;
};

