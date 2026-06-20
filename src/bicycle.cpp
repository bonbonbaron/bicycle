#include <iostream> // TODO: delete when you're done debugging
#include <chrono>
#include <thread>

#include "bicycle.h"
#include "Config.h"
#include "Constants.h"
#include "m/World.h"
#include "c/Trigger.h"
#include "c/SshInput.h"

namespace bicycle {

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

  void init( const std::string& gameName ) {
    // Configure game
    // TODO config( gameName );

    // Init ncurses
    setlocale(LC_ALL, "");
    initscr();
    start_color();
    cbreak();  // reads each character as they're typed instead of waiting for newline
    noecho();  // doesn't echo user's input back to them
    curs_set(0);
    ColorPalette::init();

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
    auto& wm = WindowManager::getInstance();

#if 1
    while (true) {
      Timer::run();
      SshInput::listen();
      wm.render();
      Timer::sleepFrame();
    }
#else
    int i{}; 
    do {
      wm.react( i );
      wm.render();
      // napms( 30 );  // delay to prevent flickering
    } while ( wm.size() > 0 && ( i = getch() ) != 'q' );
#endif

    return endwin();
  }

} // namespace bicycle
