#pragma once
#include "Window.h"
#include <array>
#include <memory>
#include <cursesw.h>

constexpr int MAX_NUM_WINDOWS{50};

// The vector has to store smart pointers because a container of superclasses would truncate subclass instances.
class WindowManager {
  public:
    static auto getInstance() -> WindowManager&  {
      static WindowManager cm;
      return cm;
    }

    void render();
    auto size() const -> int;
    auto back() const -> const std::shared_ptr<Window>;
    void push( const std::shared_ptr<Window> win );
    void pop();
    void refreshAll();
    void contextOverride( std::shared_ptr<Window> win );
    void defaultWindow();
    auto getCurrentWindow() -> std::shared_ptr<Window>;

  private:
    WindowManager() = default;
    WindowManager( const WindowManager& ) = delete;
    WindowManager& operator=( WindowManager& ) = delete;
    // Member vars
    // We have to use an array despite stack-like functionality to protect the iterator from vector/list reallocations.
    std::shared_ptr<Window> _currWindow{};
    std::array<std::shared_ptr<Window>, MAX_NUM_WINDOWS> _windows;
    int _population{};
};
