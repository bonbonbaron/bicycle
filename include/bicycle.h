#pragma once
#include <string>
#include <mutex>

#include "c/WindowManager.h"

#define EXPORT_API \
  __attribute__((used))\
  __attribute__((visibility("default")))

namespace bicycle {
  auto isInit() -> bool;
  void init( const std::string& gameName );
  void die( const std::string& s );

  Entity push( std::shared_ptr<Window> shared );
  Entity pop();
  int run();

  static std::mutex bike_mtx;

  template<typename T, typename... Args>
    Entity push( Args&&... args ) {
      // Ensure the developer remembered to initialize ncurses before adding anything to the window manager.
      // Any windows created before initialization disappear.
      auto& wm = WindowManager::getInstance();
      auto shared = std::make_shared<T>( args... );
      return wm.push( shared );  // returns new window's entity ID
    }
}  // namespace bicycle
