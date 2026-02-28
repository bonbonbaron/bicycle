#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <curses.h>
#include <functional>

#include "Window.h"
#include "Entity.h"
#include "ColorPalette.h"

struct Dimensions {
  unsigned h{};
  unsigned w{};
};

// Camera can be moved in update().
class Camera {
  public:
    Camera() = default;
    Camera( const int x, const int y, const int margin );
    void pan( const int dx, const int dy );
    auto canSee( const std::shared_ptr<Entity>& entity ) const -> bool;
    void focusOn( const std::shared_ptr<Entity> entity );
    auto getFocus() const -> std::shared_ptr<Entity>;
    void setDims( const int h, const int w);
    void setLims( const int y, const int x);
    auto getX() const -> int;
    auto getY() const -> int;
    auto getLxMargin() const -> int;  // low x margin
    auto getLyMargin() const -> int;  // low y margin
    auto getHxMargin() const -> int;  // high x margin
    auto getHyMargin() const -> int;  // high y margin
  private:
    int _x{};
    int _maxX{};
    int _y{};
    int _maxY{};
    int _w{};
    int _h{};
    int _margin{};
    std::shared_ptr<Entity> _focus{};  // input is forwarded to this guy
};

struct Environment {
  std::string bg;
  Dimensions bgDims{};
  std::map<std::string, std::shared_ptr<Entity>> fg{};
  Camera camera{ 0, 0, 3 };
  // TBD: it may be desirable to keep a list of on-screen entities. Feature it later.
};

class Grid : public Window {
  public:
    Grid( const std::string& yamlFilename );
    Grid( const int x, const int y, const int w, const int h );

    void update() override;
    void react( const int input ) override;
    void addEntity( const std::string& name, const std::shared_ptr<Entity>& entity );

  private:
    std::shared_ptr<Environment> _env{};
};

// Provide yaml-cpp library with template candidate for Quirk's specific struct
template<>
struct YAML::convert<Grid> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Grid& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    return true;
  }
};   // Grid YML conversion

