#pragma once
#include "m/Entity.h"

struct Timeout {
  Entity entity{};
  unsigned id{};      // An entity can have multiple timers running simulatneously. ID
  unsigned type{};    // I *think* this corresponds to the activity we want this to spark in recipient.
  bool isSubtimer{};  // Subtimers don't kick off new activity groups.
};

