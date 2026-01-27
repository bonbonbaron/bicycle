#include "Grid.h"

Grid::Grid( const int x, const int y, const int w, const int h ) : Window(x, y, w, h) {}

void Grid::update() {
  render();
}

void Grid::focusOn( const std::string& entityName ) {
  if ( _fg.contains( entityName ) ) {
    _focus = _fg.at( entityName );
  }
}

void Grid::render() {
  // optimize this later if it's too slow
  for ( int row = 0; row < getHeight(); ++row ) {
    auto it = _bg.begin() + ( _camera.y * getWidth() + _camera.x );
    for ( const auto start = it; it < start + getWidth(); ++it ) {
      putc( it->image );
    }
  }
}

void Grid::react( const int input ) {
  if ( _focus != nullptr ) {
    _focus->onInput( input );
  }
}
