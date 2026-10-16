#include "v/common/Window.h"
#include "bicycle.h"
#include <algorithm>
#include <cassert>

using gfx::renderer;

Window::Window( const int w, const int h ) :
  _win( renderer().createWindow( gfx::cols() / 2 - w / 2 - 2, gfx::lines() / 2 - h/2 - 2, w, h ) ), _w(w), _h(h), _id( newEntityId() ) {
  assert( gfx::lines() / 2 - h/2 - 2 > 0 );
  assert( gfx::cols() / 2 - w / 2 > 0 );
  assert( _win != nullptr );
}

Window::Window( const int x, const int y, const int w, const int h ) :
  _win( renderer().createWindow( x, y, w - 2, h ) ), _w(w - 2), _h(h), _x(x), _y(y), _id( newEntityId() ) {
  assert( _win );
}

Window::Window() : _id( newEntityId() ) {}

Window::~Window(){
  renderer().destroyWindow( _win );
  delEntityId( _id );
}

void Window::create() {
  _win = renderer().createWindow( _x, _y, _w, _h );
}

void Window::show(){
  renderer().setAttr( _win, gfx::ATTR_INVISIBLE, false );
}

void Window::hide(){
  renderer().setAttr( _win, gfx::ATTR_INVISIBLE, true );
}

void Window::clear() {
  renderer().setPanel( _win, panelKind() );
  renderer().clearWindow( _win );
}

void Window::onInput( Input& input ) {
}

auto Window::getX() const -> int{
  return renderer().windowX( _win );
}

auto Window::getY() const -> int{
  return renderer().windowY( _win );
}

auto Window::getWidth() const -> unsigned {
  return _w;
}

auto Window::getHeight() const -> unsigned {
  return _h;
}

void Window::setBorder( const bool showBorder ) {
  _showBorder = showBorder;
}

auto Window::isBorderShowing() const -> bool {
  return _showBorder;
}

void Window::repaint() {
  if ( isBorderShowing() ) {
    renderer().drawBorder( _win );
  }
  renderer().presentWindow( _win );
}

void Window::mvprint( const int y, const int x, const std::string& s ) const {
  renderer().text( _win, y, x, s );
}

void Window::place( const int x, const int y, const int w, const int h ) {
  _x = x;
  _y = y;
  _w = std::max( 1, w );
  _h = std::max( 1, h );
  renderer().resizeWindow( _win, 1, 1 );  // shrink first so the move can't push it off the screen
  renderer().moveWindow( _win, _x, _y );
  renderer().resizeWindow( _win, _w, _h );
}

// Shows the window at its requested place and size as far as the screen allows, remembering the request, so a
// window squeezed by a small terminal gets its full size back when the terminal grows.
void Window::onTermResize() {
  const int w = std::clamp( _w, 1, std::max( 1, gfx::cols() ) );
  const int h = std::clamp( _h, 1, std::max( 1, gfx::lines() ) );
  renderer().resizeWindow( _win, 1, 1 );
  renderer().moveWindow( _win, std::clamp( _x, 0, std::max( 0, gfx::cols() - w ) ),
                         std::clamp( _y, 0, std::max( 0, gfx::lines() - h ) ) );
  renderer().resizeWindow( _win, w, h );
}

void Window::setX( const int x ) {
  _x = x;
  renderer().moveWindow( _win, _x, _y );
}

void Window::setY( const int y ) {
  _y = y;
  renderer().moveWindow( _win, _x, _y );
}

void Window::setWidth( const int w ) {
  _w = w;
  renderer().resizeWindow( _win, _w, _h );
}

void Window::setHeight( const int h ) {
  _h = h;
  renderer().resizeWindow( _win, _w, _h );
}

auto Window::getContext() const -> Entity {
  return _context;
}

auto Window::getId() const -> Entity {
  return _id;
}

void Window::setAttr( const int attr ) {
  renderer().setAttr( _win, attr, true );
}

void Window::unsetAttr( const int attr ) {
  renderer().setAttr( _win, attr, false );
}
