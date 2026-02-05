#include "bicycle.h"
#include "Config.h"

namespace bicycle {
  static bool _initState{};

  void die( const std::string&& s ) {
    std::unique_lock<std::mutex> l( bike_mtx );
    endwin();
    std::cerr << "\e[91m" << s << "\e[0m\n";
    exit(1);
  }

  void die( const std::string& s ) {
    std::unique_lock<std::mutex> l( bike_mtx );
    endwin();
    std::cerr << "\e[91m" << s << "\e[0m\n";
    exit(1);
  }

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
    ColorPalette::init();

    // Configure 
    config();

    _initState = true;
    clear();
    refresh();
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
      wm.react( i );
      wm.render();
      // napms( 30 );  // delay to prevent flickering
    } while ( wm.size() > 0 && ( i = getch() ) != 'q' );

    return endwin();
  }

} // namespace bicycle
