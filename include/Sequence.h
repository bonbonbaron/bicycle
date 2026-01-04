#pragma once
#include "Event.h"

// This class is for disparate windows to appear after the previous is popped off the stack.
// It doesn't matter how many other windows stack on the previous; the next window waits for it to leave.
class Sequence : public Event {
  public:
    Sequence() = default;
    auto run() -> EventState override;
  private:
    std::vector<Event> _events;
};
