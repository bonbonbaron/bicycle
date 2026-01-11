#include <cursesw.h>
#include "WindowManager.h"

namespace bicycle {
  static bool _initState{};

  auto isInit() -> bool {
    return _initState;
  }

  void init() {
    setlocale(LC_ALL, "");
    initscr();
    start_color();
    cbreak();  // reads each character as they're typed instead of waiting for newline
    noecho();  // doesn't echo user's input back to them
    curs_set(0);
    _initState = true;
  }

  void push( std::shared_ptr<Window> shared ) {
    auto& wm = WindowManager::getInstance();
    wm.push( shared );
  }

  void pop() {
    auto& wm = WindowManager::getInstance();
    wm.pop();
  }

  int run() {
    assert( _initState == true );
    int i{};
    auto& wm = WindowManager::getInstance();

    do {
      wm.render();
    } while ( wm.size() > 0 && ( i = getch() ) != 'q' );

    return endwin();
  }

  auto getController() -> bicycle::Node& {
    auto& wm = WindowManager::getInstance();
    return wm.getController();
  }

} // namespace bicycle
