#include "WindowManager.h"

auto WindowManager::back() const -> const std::shared_ptr<Window> {
  if ( _population > 0 ) {
    return _windows.at(_population - 1);
  }
  return {};
}

void WindowManager::push( const std::shared_ptr<Window> win ) {
  std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  win->setBorder(true);
  if ( _population < _windows.size() ) {
    _windows.at(_population++) = win;
  }
}

void WindowManager::pop() {
  std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  if ( _population > 0 ) {
    back()->clear();
    back()->setBorder(false);
    back()->repaint();
    _windows.at(--_population) = nullptr;
  }
}

void WindowManager::refreshAll() {
  for (int i = 0; i < _population; ++i ) {
    if ( _windows.at(i) != nullptr ) {
      _windows.at(i)->clear();
      _windows.at(i)->update();
      _windows.at(i)->repaint();
    }
  }
}

auto WindowManager::size() const -> int {
  return _population;
}

void WindowManager::react( const int input ) {
  // std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  if ( auto w = back() ) {
    w->react( input );  // Let the topmost window alone receive key-presses.
  }
}

void WindowManager::render() {
  std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  refreshAll();  // clears, updates, and repaints each window prior to displaying
  doupdate();  // displays results of the above window-painting
}

