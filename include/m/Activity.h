#pragma once
#include "m/Personality.h"

struct Activity {
  decltype(Quirk::action) callback{};
  decltype(Quirk::priority) priority{};
  decltype(Quirk::freq) freq{};
  decltype(Quirk::reps) nReps{};
  decltype(Quirk::reps) nRepsRemaining{};
  ActionState state{};

  auto operator=( const Quirk& q ) -> Activity {
    Activity a;  // The 3 trigger types will know which variant this is.
    a.callback = q.action;
    a.priority = q.priority;
    a.freq = q.freq;
    a.nReps = q.reps;
    a.nRepsRemaining = q.reps;
    return a;
  }
};

