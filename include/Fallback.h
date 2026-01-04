#pragma once
#include "Event.h"
#include <memory>

// This class is for disparate windows to appear after the previous is popped off the stack.
// It doesn't matter how many other windows stack on the previous; the next window waits for it to leave.
class Fallback : public Event {
  public:
    Fallback() = default;
    auto run() -> EventState override;
    void reset() override;
  private:
    std::vector<std::shared_ptr<Event>> _events{};
};
