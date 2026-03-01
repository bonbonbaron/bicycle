#include <iostream>
#include "m/World.h"
#include "c/Timer.h"
#include <chrono>
#include <thread>
#include "Constants.h"

using namespace std;

int main()
{
  using namespace std::chrono_literals;

  std::chrono::milliseconds interval(static_cast<int>(MILLISECONDS_PER_FRAME));
  Timer t;
  auto timerId = t.start( 500, 1, "digidigi", -1 );

  int i = 0; 
  while (true)
  {
    auto start = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::steady_clock::now() - start;
    t.run();
    if (elapsed < interval) {
      std::this_thread::sleep_for(interval - elapsed);
    }
    else {
      std::cout << "went overboard\n";
    }
    if (++i == 100 ) {
      t.pause(timerId);
    }
    else if ( i == 200 ) {
      t.unpause(timerId);
      t.setDuration( timerId, 333 );
    }
    else if ( i == 300 ) {
      t.stop( timerId );
    }
  }
}
