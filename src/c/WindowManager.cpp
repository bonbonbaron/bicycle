#include "c/WindowManager.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

auto WindowManager::back() const -> const std::shared_ptr<Window> {
  if ( _population > 0 ) {
    return _windows.at(_population - 1);
  }
  return {};
}

Entity WindowManager::push( const std::shared_ptr<Window> win ) {
  std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  win->setBorder(true);
  if ( _population < _windows.size() ) {
    _windows.at(_population++) = win;
  }
  return win->getId();
}

Entity WindowManager::pop() {
  Entity nextWindowId{};
  std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  if ( _population > 0 ) {
    back()->clear();
    back()->setBorder(false);
    back()->repaint();
    _windows.at(--_population) = nullptr;
  }
  if ( _population > 0 ) {
    nextWindowId = back()->getId();
  }
  return nextWindowId;
}

// ncurses only notices a resized terminal through getch(), which bicycle doesn't use, so check for it here.
static auto termResized() -> bool {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO info{};
  if ( !GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE ), &info ) ) {
    return false;
  }
  const int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
  const int cols = info.srWindow.Right - info.srWindow.Left + 1;
#else
  winsize ws{};
  if ( ioctl( STDOUT_FILENO, TIOCGWINSZ, &ws ) != 0 ) {
    return false;
  }
  const int rows = ws.ws_row;
  const int cols = ws.ws_col;
#endif
  if ( rows <= 0 || cols <= 0 || ( rows == LINES && cols == COLS ) ) {
    return false;
  }
  resize_term( rows, cols );
  clear();
  return true;
}

void WindowManager::refreshAll() {
  if ( termResized() ) {
    for ( unsigned i = 0; i < _population; ++i ) {
      if ( _windows.at(i) != nullptr ) {
        _windows.at(i)->onTermResize();
      }
    }
    wnoutrefresh( stdscr );
  }
  for (unsigned i = 0; i < _population; ++i ) {
    if ( _windows.at(i) != nullptr ) {
      _windows.at(i)->clear();
      _windows.at(i)->render();
      _windows.at(i)->repaint();
    }
  }
  doupdate();  // displays results of the above window-painting
}

auto WindowManager::size() const -> int {
  return _population;
}

void WindowManager::render() {
  std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  refreshAll();  // clears, updates, and repaints each window prior to displaying
}

