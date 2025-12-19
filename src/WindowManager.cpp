#include "WindowManager.h"

auto WindowManager::back() const -> const std::shared_ptr<Window> {
  if ( _population > 0 ) {
    return _windows.at(_population - 1);
  }
  return {};
}

void WindowManager::push( const std::shared_ptr<Window> win ) {
  if ( _population < _windows.size() ) {
    _windows.at(_population++) = win;
    _currContext = win;
  }
}

void WindowManager::pop() {
  if ( _population > 0 ) {
    _windows.at(--_population) = nullptr;
    defaultContext();
  }
}

void WindowManager::refreshAll() {
  for (int i = 0; i < _population; ++i ) {
    _windows.at(i)->clear();  // is this necessary after bicycle::run()'s erase()?
    // If you add a window from within this function, the iterator's not smart enough to know to stop.
    _windows.at(i)->update();
    // If update() didn't pop this window, repaint it.
    if ( _windows.at(i) != nullptr ) {
      _windows.at(i)->repaint();
    }
  }
  // Deferred adds are handled here.
}


auto WindowManager::getCurrentContext() -> std::shared_ptr<Window> {
  return _currContext;
}

void WindowManager::contextOverride( std::shared_ptr<Window> win ) {
  _currContext = win;
}

void WindowManager::defaultContext() {
  _currContext = back();
}


auto WindowManager::size() const -> int {
  return _population;
}
