#pragma once
#include <string>
#include <mutex>

#include "c/WindowManager.h"

#include "export.h"

namespace bicycle {
  void init( const std::string& gameName );
  void die( const std::string& s );

  Entity push( std::shared_ptr<Window> shared );
  Entity pop();
  int run();

  static std::mutex bike_mtx;

  template<typename T, typename... Args>
    Entity push( Args&&... args ) {
      // A new window of type T on top of the stack; returns its entity id.
      auto& wm = WindowManager::getInstance();
      auto shared = std::make_shared<T>( args... );
      return wm.push( shared );
    }
}  // namespace bicycle
