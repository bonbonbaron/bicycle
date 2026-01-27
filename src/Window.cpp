#include "Window.h"
#include <cassert>
#include <iostream>
#include <iomanip>

Window::Window( const int w, const int h ) : _win( newwin( h, w, LINES / 2 - h/2 - 2, COLS / 2 - w / 2 ) ), _w(w), _h(h) {
  assert( LINES / 2 - h/2 - 2 > 0 );
  assert( COLS / 2 - w / 2 > 0 );
  assert( _win != nullptr );
}

Window::Window( const int x, const int y, const int w, const int h ) : _win( newwin( h, w, y, x ) ), _w(w), _h(h) {
  assert( _win );
  const auto& cp = ColorPalette::getInstance();
  _colorPair = cp.getColorPair( WINDOW_ );
}

Window::~Window(){
  delwin( _win );
}

void Window::show(){
  wattroff( _win, A_INVIS );
}

void Window::hide(){
  wattron( _win, A_INVIS );
}

void Window::clear() {
  wclear( _win );
}

auto Window::getX() const -> int{
  return getbegx( _win );
}

auto Window::getY() const -> int{
  return getbegy( _win );
}

auto Window::getWidth() const -> int{
  return _w;
}

auto Window::getHeight() const -> int{
  return _h;
}

void Window::setColorPair( const ColorPair cp ) {
  _colorPair = COLOR_PAIR( cp );
}

auto Window::getColorPair() const -> chtype {
  return _colorPair;
}

void Window::setBorder( const bool showBorder ) {
  _showBorder = showBorder;
}

auto Window::isBorderShowing() const -> bool {
  return _showBorder;
}

void Window::repaint() {
  box( _win, 0, 0 );
  wnoutrefresh( _win );
}

void Window::mvprint( const std::string& s, const int x, const int y ) const {
  mvwprintw( _win, y, x, "%s", s.c_str() );
}

// Shall we assume it's always from the beginning?
void Window::print( const std::string& s ) const {
  wprintw( _win, "%s", s.c_str() );
}
