#pragma once
#include <memory>
#include <queue>
#include "Window.h"
#include "bicycle.h"

// This class is for disparate windows to appear after the previous is popped off the stack.
// It doesn't matter how many other windows stack on the previous; the next window waits for it to leave.
class Sequence {
  public:
    Sequence() = default;
    void push( std::shared_ptr<Window> win );
    void pop();
    void tick();
    auto size() -> int;
    auto isComplete() -> bool;
    template<typename T, typename... Args>
      void push( Args&&... args ) {
        auto shared = std::make_shared<T>( args... );
        push( shared );
      }
  private:
    std::weak_ptr<Window> _currWindow{};
    std::queue<std::shared_ptr<Window>> _windows{};
};
