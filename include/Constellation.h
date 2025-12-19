#pragma once
#include <map>
#include <string>
#include <vector>
#include "Window.h"
#include <functional>

struct Point {
  Color color;         // color it's given (duh)
  int x, y;            // where this symbol draws relative to the window
  std::string symbol;  // how it appears on the screen
};

struct NamedPoint {
  NamedPoint( const std::string name, const Color color, const int x, const int y, const std::string symbol ) :
    name(name),
    point(color, x, y, symbol ) {}
  std::string name;
  Point point;
};

using SelectionCallback = std::function<void()>;
using PointMap = std::map<std::string, Point>;

class Constellation : public Window {
  public:
    Constellation( const int w, const int h );
    Constellation( const int x, const int y, const int w, const int h );
    Constellation( const std::vector<NamedPoint>& namepoints, const int x, const int y, const int w, const int h );
    Constellation( const std::vector<NamedPoint>&& namepoints, const int x, const int y, const int w, const int h );

    auto getPoints() const -> const PointMap&;
    void removePoint( const std::string& key );
    void setPoint( const std::string& name, const Point& pos );
    void setPoint( const std::string&& name, const Point&& pos );
    void react( const int input ) override;
    void update() override;
    auto getSelectedPoint() const -> const Point&;
    auto getSelectedPointName() const -> const std::string;
    void setSelecting( const bool selecting );
    auto isSelecting() const -> bool;
    void setCallback( SelectionCallback selCallback);
  private:
    PointMap _points{};
    bool _selecting{};
    int _idx{};
    std::string _selectedKey;
    SelectionCallback _selCallback{};
};
    
