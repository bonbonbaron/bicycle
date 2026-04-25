#pragma once
#include <vector>

// TODO wait till core Activity takes shape before you worry about orchestrating stopping components.
class Activity {
  public:
    static auto getInstance() -> Activity&;
    void run();
  private:
    Activity();
    Activity(const Activity&) = delete;
    Activity operator=(const Activity&) = delete;
    Activity(const Activity&&) = delete;
    Activity operator=(const Activity&&) = delete;

    struct Quirk {
      Acit tree{};
      int priority{};  // higher values take precedence
      unsigned freq{};  // freq at quirk-level gives entities more ownership over their own rates
      unsigned reps{};  // freq at quirk-level gives entities more ownership over their own rates
    };

    struct ActionStatus {
      

    struct ActiveComponents {
      std::vector timerIds{};
      // TODO add more here as your engine matures
    };

    /

    // members
};  // class Activity
