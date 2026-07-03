#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <curses.h>
#include <functional>
#include <memory>

#include "v/Window.h"
#include "m/Entity.h"
#include "m/Grid.h"
#include "m/Camera.h"

class Scene : public Window {
  public:
    Scene( const Grid& grid );
    Scene( const int x, const int y, const int w, const int h );

    void render() override;
    void onInput( const InputState& input ) override;

  private:
    Grid _grid{};    // This has layers of BGs and FGs.
    Camera _camera{};  // TODO  when we have minimaps, the input needs to go to a window BELOW the top of WindowManager's stack.
};
