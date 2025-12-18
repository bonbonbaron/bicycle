#include <map>
#include <string>
#include <vector>
#include "Window.h"

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

  private:
    PointMap _points{};
};
    
