#pragma once
#include <string>
#include <cassert>
#include "WindowManager.h"

namespace bicycle {
  auto isInit() -> bool;
  void init();

  void push( std::shared_ptr<Window> shared );
  void pop();
  int run();
  auto getController() -> bicycle::Node&;

  template<typename T, typename... Args>
    void push( Args&&... args ) {
      // Ensure the developer remembered to initialize ncurses before adding anything to the window manager.
      // Any windows created before initialization disappear.
      assert( isInit() );
      auto& wm = WindowManager::getInstance();
      auto shared = std::make_shared<T>( args... );
      wm.push( shared );
    }
}  // namespace bicycle
