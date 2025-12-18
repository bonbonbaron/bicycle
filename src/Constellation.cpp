#include "Constellation.h"
#include <cassert>

Constellation::Constellation( const int w, const int h ) : Window( w, h ) {}

Constellation::Constellation( const int x, const int y, const int w, const int h ) : Window( x, y, w, h ) {}

Constellation::Constellation( const std::vector<NamedPoint>& namepoints, const int x, const int y, const int w, const int h ) : Window( x, y, w, h ) {
  for ( const auto& n : namepoints ) {
    _points[ n.name ] = n.point;
  }
}

Constellation::Constellation( const std::vector<NamedPoint>&& namepoints, const int x, const int y, const int w, const int h ) : Window( x, y, w, h ) {
  for ( const auto& n : namepoints ) {
    _points[ n.name ] = n.point;
  }
}


auto Constellation::getPoints() const -> const PointMap& {
  return _points;
}

void Constellation::setPoint( const std::string& name, const Point& point ) {
  _points[ name ] = (point);
}

void Constellation::setPoint( const std::string&& name, const Point&& point ) {
  _points[ name ] = (point);
}

void Constellation::setCallback( SelectionCallback selCallback) {
  _selCallback = selCallback;
}

void Constellation::react( const int input ) {
  if ( _selecting ) {
    switch ( input ) {
      case 'j':
      case 'J':
      case 'h':
      case 'H':
        --_idx;
        if ( _idx < 0 ) {
          _idx = _points.size() - 1;
        }
        break;
      case 'k':
      case 'K':
      case 'l':
      case 'L':
        ++_idx;
        if ( _idx >= _points.size() ) {
          _idx = 0;
        }
        break;
      case ' ':
        setSelecting( false );
    }
  }
}

auto Constellation::getSelectedPoint() const -> const Point& {
  auto it = _points.begin();
  assert ( _idx >= 0 && _idx < _points.size() );
  for ( int i = 0; i < _idx; ++i, it++ );
  return it->second;
}

void Constellation::setSelecting( const bool selecting ) {
  _selecting = selecting;
}

auto Constellation::isSelecting() const -> bool {
  return _selecting;
}

void Constellation::update() {
  for ( const auto& p : _points ) {
    print( p.second.symbol, p.second.x, p.second.y );
  }
  if ( _selecting ) {
    auto it = _points.begin();
    for ( int i = 0; i < _idx; ++i, it++ );
    mvprintw( it->second.y, it->second.x - 2, ">" );
  }
}

void Constellation::removePoint( const std::string& key ) {
  auto it = _points.find( key );
  if ( it != _points.end() ) {
    _points.erase( it );
  }
}
