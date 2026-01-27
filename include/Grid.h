#pragma once
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

    std::vector<Tile> _bg;
    std::map<std::string, std::shared_ptr<Entity>> _fg;
    std::shared_ptr<Entity> _focus{};  // input is forwarded to this guy

    Camera _camera;
};

