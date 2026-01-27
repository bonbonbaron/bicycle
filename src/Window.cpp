#include "Window.h"
#include "bicycle.h"
#include <cassert>

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

void Window::create() {
  _win = newwin( _h, _w, _y, _x );
}

void Window::show(){
  wattroff( _win, A_INVIS );
}

void Window::hide(){
  wattron( _win, A_INVIS );
}

void Window::clear() {
  werase( _win );
}

void Window::react( const int input ) {
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
  if ( isBorderShowing() ) {
    box( _win, 0, 0 );
  }
  wnoutrefresh( _win );
}

void Window::mvprint( const int y, const int x, const std::string& s ) const {
  mvwprintw( _win, y, x, "%s", s.c_str() );
}

void Window::addmvch( const int y, const int x, char c ) const {
  mvwaddch( _win, y, x, c );
}

void Window::putc( const char c ) const {
  int error = waddch( _win, c );
  if ( error ) {
    bicycle::die( "error calling addch() in Grid::render()" );
  }
}

void Window::move( int y, int x ) {
  wmove( _win, y, x );
}

// Shall we assume it's always from the beginning?
void Window::print( const std::string& s ) const {
  wprintw( _win, "%s", s.c_str() );
}


void Window::setX( const int x ) {
  _x = x;
  mvwin( _win, _x, _y );
}

void Window::setY( const int y ) {
  _y = y;
  mvwin( _win, _x, _y );
}

void Window::setWidth( const int w ) {
  _w = w;
  wresize( _win, _h, _w );
}

void Window::setHeight( const int h ) {
  _h = h;
  wresize( _win, _h, _w );
}

