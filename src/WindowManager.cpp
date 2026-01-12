#include "WindowManager.h"

auto WindowManager::back() const -> const std::shared_ptr<Window> {
  if ( _population > 0 ) {
    return _windows.at(_population - 1);
  }
  return {};
}

void WindowManager::push( const std::shared_ptr<Window> win ) {
  std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  if ( _population < _windows.size() ) {
    _windows.at(_population++) = win;
    _currWindow = win;
  }
}

void WindowManager::pop() {
  std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  if ( _population > 0 ) {
    _windows.at(--_population) = nullptr;
    defaultWindow();
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

auto WindowManager::size() const -> int {
  return _population;
}

void WindowManager::render() {
  std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  erase();
  refresh();  // erase() interferes with consequent rendering if it isn't triggered here.
  _windows.back()->react( i );  // Let the topmost window alone receive key-presses.
  refreshAll();  // clears, updates, and repaints each window prior to displaying
  doupdate();  // displays results of the above window-painting
}

