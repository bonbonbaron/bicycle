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

struct Tile {
  unsigned char type;  // to instruct what to do on collision
  unsigned char image;
  Color color;
};

/* TODO
 * detect collisions
 * forward inputs to whomever has the focus
 * receive  bg from file
 * overlay fg on bg
 * color units based on color map
 */

class Grid : public Window {
  public:
    Grid( const std::string& yamlFilename );
    Grid( const int x, const int y, const int w, const int h );

    void update() override;
    void react( const int input ) override;
    void render();
    void focusOn( const std::string& entityName );

  private:
    // Camera can be moved in update().
    struct Camera {
      int x{};
      int y{};
    };

    Dimensions _bgDims;

    std::string _bg;
    std::map<std::string, std::shared_ptr<Entity>> _fg;
    std::shared_ptr<Entity> _focus{};  // input is forwarded to this guy

    Camera _camera;
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

