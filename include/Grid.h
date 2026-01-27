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
    Grid();  //  this is what the thing wants
    Grid( const int x, const int y, const int w, const int h );

    void update() override;
    void update() override;
  private:
    std::vector<Tile> bg;
    std::vector<Entity> fg;

    // Camera can be moved in update().
    struct Camera {
      int x{};
      int y{};
      Entity target{};
      void render( Window* window );
    };

    Camera _camera;

};


