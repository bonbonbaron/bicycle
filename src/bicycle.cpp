#include <cursesw.h>
#include "WindowManager.h"
#include "Context.h"

namespace bicycle {
  static bool initState{};

  auto isInit() -> bool {
    return initState;
  }

  void init_ncurses() {
    setlocale(LC_ALL, "");
    initscr();
    start_color();
    cbreak();  // reads each character as they're typed instead of waiting for newline
    noecho();  // doesn't echo user's input back to them
    curs_set(0);
    initState = true;
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
    assert( initState == true );
    int i{};
    auto& cm = WindowManager::getInstance();

    do {
      erase();
      refresh();  // erase() interfere's with consequent rendering if it isn't triggered here.
      cm.getCurrentContext()->react( i );  // Let the topmost window alone receive key-presses.
      cm.refreshAll();  // clears, updates, and repaints each window prior to displaying
      mvprintw( 1, 1, "Number of windows on bicycle stack: %d", cm.size() );
      doupdate();  // displays results of the above window-painting
    } while ( cm.size() > 0 && ( i = getch() ) != 'q' );

    return endwin();
  }
} // namespace bicycle
