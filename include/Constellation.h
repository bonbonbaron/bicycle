#pragma once
#include <map>
#include <string>
#include <vector>
#include "Window.h"
#include <functional>
#include <optional>
#include <cassert>

// I think it has to be a class not struct to inherit enabled proeprly.
template<typename T, typename... Args>
class Point : public T {
  public:
    Point( Args&... args ) : T( args... ) {};
    Color color{};         // color it's given (duh)
    bool good{};         // are you a GOOD boy or a BAD boy??
    int x{}, y{};            // where this symbol draws relative to the window
    std::string symbol;  // how it appears on the screen
};

template<typename T>
using PointMap = std::map<std::string, struct Point<T>>;

template<typename T>
class Constellation : public Window {

  struct NamedPoint {
    NamedPoint( const std::string name, const Color color, const int x, const int y, const std::string symbol ) :
      name(name),
      point(color, x, y, symbol ) {}
    std::string name;
    Point<T> point;
  };

  using SelectionCallback = std::function<void()>;

  public:

  Constellation( const int w, const int h ) : Window( w, h ) {}

  Constellation( const int x, const int y, const int w, const int h ) : Window( x, y, w, h ) {}

  Constellation( const std::vector<NamedPoint>& namepoints, const int x, const int y, const int w, const int h ) : Window( x, y, w, h ) {
    for ( const auto& n : namepoints ) {
      _points[ n.name ] = n.point;
    }
  }

  Constellation( const std::vector<NamedPoint>&& namepoints, const int x, const int y, const int w, const int h ) : Window( x, y, w, h ) {
    for ( const auto& n : namepoints ) {
      _points[ n.name ] = n.point;
    }
  }

  void add( const T& t, const std::string name, const int x, const int y, const Color color = Color::WHITE ) {
    _points[ name ] = Point<T>( color, x, y, name, t );
  }

  void add( const T& t, const std::string name, const std::string symbol, const int x, const int y, const Color color = Color::WHITE ) {
    _points[ name ] = Point<T>( color, x, y, symbol, t );
  }

  auto getPoints() const -> const PointMap<T>& {
    return _points;
  }

  void setPoint( const std::string& name, const Point<T>& point ) {
    _points[ name ] = (point);
  }

  void setPoint( const std::string&& name, const Point<T>&& point ) {
    _points[ name ] = (point);
  }

  void setCallback( SelectionCallback selCallback) {
    _selCallback = selCallback;
  }

  void react( const int input ) {
    auto it = _points.begin();
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
          for ( int i = 0; i < _idx; ++i, it++ );
          _selectedKey = it->first;
          setSelecting( false );
      }
    }
  }

  auto getSelectedPointName() const -> const std::optional<std::string> {
    // If we're busy selecting something, we shouldn't be able to get back a selection yet.
    if ( !_selecting ) {
      auto it = _points.find( _selectedKey );
      if ( it != _points.end() ) {
        return {it->first};
      }
    }
    return {};
  }

  auto getSelectedPoint() const -> const std::optional<Point<T>&> {
    // If we're busy selecting something, we shouldn't be able to get back a selection yet.
    if ( !_selecting ) {
      auto it = _points.find( _selectedKey );
      if ( it != _points.end() ) {
        return {it->second};
      }
    }
    return {};
  }

  void setSelecting( const bool selecting ) {
    _selecting = selecting;
  }

  auto isSelecting() const -> bool {
    return _selecting;
  }

  void update() {
    for ( const auto& p : _points ) {
      print( p.second.symbol, p.second.x, p.second.y );
    }
    if ( _selecting ) {
      auto it = _points.find( _selectedKey );
      assert( it != _points.end() );
      mvprintw( it->second.y, it->second.x - 2, ">" );
    }
  }

  auto getPoint( const std::string& name ) -> std::optional<Point<T>&> {
    auto it = _points.find( name );
    if ( it != _points.end() ) {
      return {it->second};
    }
    return {};
  }

  // The user may want to extract specific members into a coherent group.
  auto filter( std::function<bool(Point<T>&)> f ) -> PointMap<T> {
    PointMap<T> newMap{};
    for ( auto& c : _points ) {
      if ( f( c.second ) == true ) {
        newMap[ c.first ] = c.second;
      }
    }
    return newMap;
  }

  private:
  PointMap<T> _points{};
  bool _selecting{ false };
  int _idx{};
  std::string _selectedKey;
  SelectionCallback _selCallback{};
};

