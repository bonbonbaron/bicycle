#include <cursesw.h>
#include "WindowManager.h"

namespace bicycle {
  static bool _initState{};

  auto isInit() -> bool {
    return _initState;
  }

  void init_ncurses() {
    setlocale(LC_ALL, "");
    initscr();
    start_color();
    cbreak();  // reads each character as they're typed instead of waiting for newline
    noecho();  // doesn't echo user's input back to them
    curs_set(0);
    _initState = true;
  }

  void pop() {
    auto& wm = WindowManager::getInstance();
    wm.pop();
  }

  void push( std::shared_ptr<Window> shared ) {
    auto& wm = WindowManager::getInstance();
    wm.push( shared );
  }

  // Battle is pushed to bicyle stack before this.
  int run() {
    // Ensure the developer remembered to initialize ncurses before running the bicycle engine.
    assert( _initState == true );
    int i{};
    auto& wm = WindowManager::getInstance();

    do {
      wm.render();
    } while ( wm.size() > 0 && ( i = getch() ) != 'q' );

    return endwin();
  }
} // namespace bicycle
