#include "Constellation.h"

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

void Constellation::react( const int input ) {
  // Do nothing for now at least. Probably will forever do nothing, or at least let a child class fulfill it.
}

void Constellation::update() {
  for ( const auto& p : _points ) {
    print( p.second.symbol, p.second.x, p.second.y );
  }
}

void Constellation::removePoint( const std::string& key ) {
  auto it = _points.find( key );
  if ( it != _points.end() ) {
    _points.erase( it );
  }
}
