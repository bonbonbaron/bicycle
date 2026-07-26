#pragma once
#include "m/Entity.h"

enum TimeoutAddr { BRIDGE, ANIMATION, RENDERING, MOTION };  // Notice how this only addresses outputs. Cool.

struct Timeout {
  Entity entity{};
  unsigned id{};      // An entity can have multiple timers running simulatneously. ID
  unsigned type{};    // I *think* this corresponds to the activity we want this to spark in recipient.
  TimeoutAddr addr{BRIDGE};
};

