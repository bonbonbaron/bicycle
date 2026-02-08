#pragma once
#include <string>
#include <mutex>

#include "WindowManager.h"

namespace bicycle {
  auto isInit() -> bool;
  void init( const std::string& gameName );
  void die( const std::string& s );

  void push( std::shared_ptr<Window> shared );
  void pop();
  int run( const std::string& gameName );

  static std::mutex bike_mtx;

  template<typename T, typename... Args>
    void push( Args&&... args ) {
      // Ensure the developer remembered to initialize ncurses before adding anything to the window manager.
      // Any windows created before initialization disappear.
      auto& wm = WindowManager::getInstance();
      auto shared = std::make_shared<T>( args... );
      wm.push( shared );
    }
}  // namespace bicycle
