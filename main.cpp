#include <iostream>
#include "m/World.h"
#include "c/Activity.h"
#include "c/SshInput.h"
#include <chrono>
#include <thread>
#include "Constants.h"

int main()
{
  int i = 0; 
  while (true) {
    // General loop-regulating
    auto start = std::chrono::steady_clock::now();

    // Timer stuff
    Timer::run();
    Input::listen();

    // Action stuff
    // TODO

    auto elapsed = std::chrono::steady_clock::now() - start;
    // General loop-regulating
    if (elapsed < interval) {
      std::this_thread::sleep_for(interval - elapsed);
    }
  }
}
