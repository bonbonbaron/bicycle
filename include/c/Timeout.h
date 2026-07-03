#pragma once
#include "m/Entity.h"

struct Timeout {
  unsigned id{};  // An entity can have multiple timers running simulatneously. ID
  Entity entity{};
  unsigned type{};  // B
  std::string msg;  // TODO replace with a proper enum
};

