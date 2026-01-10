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
    auto& wm = WindowManager::getInstance();

    // Hypothesis: There'll only ever be one node active. Nodes contain business logic, or "controller" in MVC-speak.
    //             So, being in charge of springing the windows up, it'll update before the windows do.
    //             But if a menu has control, that's where that line of thinking breaks down. 
    //             Perhaps better: NOdes could extend Context, and we'd change WindowManager to a ContextManager.
    //
    do {
      erase();
      refresh();  // erase() interferes with consequent rendering if it isn't triggered here.
      wm.getCurrentContext()->react( i );  // Let the topmost window alone receive key-presses.
      wm.refreshAll();  // clears, updates, and repaints each window prior to displaying
      doupdate();  // displays results of the above window-painting
    } while ( wm.size() > 0 && ( i = getch() ) != 'q' );

    return endwin();
  }
} // namespace bicycle
