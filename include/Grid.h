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

class Grid : public Window {
  public:
    Grid( const std::string& yamlFilename );
    Grid( const int x, const int y, const int w, const int h );

    void update() override;
    void react( const int input ) override;
    void render();
    void focusOn( const std::string& entityName );
    auto isOnscreen( const std::shared_ptr<Entity>& entity ) -> bool;
    void pan( const int dx, const int dy );
    void addEntity( const std::string& name, const std::shared_ptr<Entity>& entity );

  private:
    // Camera can be moved in update().
    struct Camera {
      int x{};
      int y{};
      int margin{3};
    };

    Dimensions _bgDims;

    std::string _bg;
    std::map<std::string, std::shared_ptr<Entity>> _fg;
    std::shared_ptr<Entity> _focus{};  // input is forwarded to this guy

    Camera _camera{};
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

