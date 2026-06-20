#pragma once
#include "m/Personality.h"
#include "c/Timer.h"
#include <vector>
#include <set>

struct Activity : public Quirk {
  decltype(Quirk::reps) nRepsRemaining{};
  ActionState state{};
  std::vector<Activity> children{};
  std::set<Timer> timers{};  // should only be populated for the top level

  auto operator=( const Quirk& q ) -> Activity {
    Activity a;  // The 3 trigger types will know which variant this is.
    a.action = q.action;
    a.priority = q.priority;
    a.freq = q.freq;
    a.reps = q.reps;
    a.nRepsRemaining = q.reps;
    return a;
  }
};

