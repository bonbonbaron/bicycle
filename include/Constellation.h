#pragma once
#include <map>
#include <string>
#include <vector>
#include "Window.h"
#include <functional>
#include <optional>


template<typename T>
class Constellation : public Window {
  struct Point {
    Color color;         // color it's given (duh)
    int x, y;            // where this symbol draws relative to the window
    std::string symbol;  // how it appears on the screen
    T userData;
  };

  struct NamedPoint {
    NamedPoint( const std::string name, const Color color, const int x, const int y, const std::string symbol ) :
      name(name),
      point(color, x, y, symbol ) {}
    std::string name;
    Point point;
  };

  using SelectionCallback = std::function<void()>;
  using PointMap = std::map<std::string, struct Point>;

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


  auto getPoints() const -> const PointMap& {
    return _points;
  }

  void setPoint( const std::string& name, const Point& point ) {
    _points[ name ] = (point);
  }

  void setPoint( const std::string&& name, const Point&& point ) {
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

  auto getSelectedPoint() const -> const std::optional<Point&> {
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

  void removePoint( const std::string& key ) {
    auto it = _points.find( key );
    if ( it != _points.end() ) {
      _points.erase( it );
    }
  }

  auto getUserData( const std::string& name ) -> std::optional<T&> {
    auto it = _points.find( name );
    if ( it != _points.end() ) {
      return {it->second};
    }
    return {};
  }

  private:
  PointMap _points{};
  bool _selecting{};
  int _idx{};
  std::string _selectedKey;
  SelectionCallback _selCallback{};
};

