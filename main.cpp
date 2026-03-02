#include <iostream>
#include "m/World.h"
#include "c/Timer.h"
#include "c/Input.h"
#include <chrono>
#include <thread>
#include "Constants.h"

int main()
{
  using namespace std::chrono_literals;
  KeyboardListener kb;

  std::chrono::milliseconds interval(static_cast<int>(MILLISECONDS_PER_FRAME));
  Timer t;
  auto timerId = t.start( 500, 1, "digidigi", -1 );

  int i = 0; 
  while (true) {
    // General loop-regulating
    auto start = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::steady_clock::now() - start;

    // Timer stuff
    t.run();
    Input::listen();

    // General loop-regulating
    if (elapsed < interval) {
      std::this_thread::sleep_for(interval - elapsed);
    }
  }
}
