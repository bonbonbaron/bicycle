#include <iostream> // TODO: delete when you're done debugging
#include "bicycle.h"
#include "Constants.h"
#include "c/Trigger.h"
#include "c/SshInputListener.h"
#include "c/CollisionDetector.h"
#include "v/Dialogue.h"
#include "v/TextMenu.h"
#include "v/Scene.h"

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

    auto& trig = Trigger::getInstance();
    trig.init( gameName );
  }

  Entity push( std::shared_ptr<Window> shared ) {
    auto& wm = WindowManager::getInstance();
    return wm.push( shared );
  }

  extern "C" {
    Entity pushDialogue( const char* text, int x, int y, int w, int h ) {
      return push<Dialogue>( text, x, y, w, h ); 
    }

    Entity pushTextMenu( int x, int y, int w, int h ) {
      return push<TextMenu>( x, y, w, h ); 
    }

    void addItem( const char* text ) {
      auto& wm = WindowManager::getInstance();
      auto menu = dynamic_pointer_cast<Menu>( wm.back() );
      if ( menu != nullptr ) {
        menu->addItem( text );
      }
    }

    Entity pushScene() {
      return push<Scene>();
    }

    // TODO handle collision string and type later
    void addLayer( const char* bgStr ) {
      auto& wm = WindowManager::getInstance();
      auto scene = dynamic_pointer_cast<Scene>( wm.back() );
      auto& grid = scene->getGrid();
      Layer layer{ std::string( bgStr ), "", LayerType::FIXED };  // TODO
      grid.addLayer( layer );
    }

    Entity popWindow() {
      return pop();
    }

    unsigned getSelection() {
      auto& wm = WindowManager::getInstance();
      auto menu = dynamic_pointer_cast<Menu>( wm.back() );
      if ( menu != nullptr ) {
        return menu->getSelection();
      }
      return 0;  // i'm tired, i'll make this -1 (int, not unsigned) later
    }
      
  }

  Entity pop() {
    auto& wm = WindowManager::getInstance();
    return wm.pop();
  }

  int run() {
    auto& wm = WindowManager::getInstance();
    auto& trigger = Trigger::getInstance();

    while ( wm.size() > 0 ) {
      Timer::run();
      SshInput::listen();
      CollisionDetector::check();
      trigger.send();
      wm.render();   // TODO make this internally only change dirty windows
      Timer::sleepFrame();
    }

    return endwin();
  }

} // namespace bicycle
