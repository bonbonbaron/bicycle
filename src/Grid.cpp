#include "Grid.h"

Grid::Grid( const int x, const int y, const int w, const int h ) : Window(x, y, w, h) {}

void Grid::update() {
  _camera.render( (Window*) this );
}

void Grid::Camera::render( Window* window ) {
  // optimize this later if it's too slow
  for ( int row = 0; row < window->getHeight(); ++row ) {
    window->print( "hello idiot" );
  }
}
