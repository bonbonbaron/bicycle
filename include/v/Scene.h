#pragma once
#include <string>
#include <vector>
#include <curses.h>
#include <optional>
#include <unordered_map>

#include "m/Camera.h"
#include "m/Rect.h"
#include "m/Entity.h"

#include "v/Window.h"

#include "LineLimits.h"

struct Layer {
  enum Type { FIXED, UNIFORM, PARALLAX, AUTOLOOP };

  Layer();
  Layer( const std::string bgStr = "", const std::string& bgCollStr = "", Layer::Type type = Layer::Type::UNIFORM );
  Entity id{};
  std::string bgStr;
  std::string bgCollStr;
  std::vector<LineLimits> lineLimits{};
  // Things Layer needs to have in World:
  //   * \ entity ID
  //   * image
  //   * position (if PARALLAX or AUTOLOOP)
  //   * velocity (if AUTOLOOP)
  // Backgrounds can have positions other than (0,0) for parallax or auto-looping
  // They may need to pass an Image in too.
  Type type{};
  std::vector<Entity> fg{};  // entities in the foreground
  // TODO some timer tracker to know which BG tile to animate next
};

class Grid {
  public:
    void addEntity( const Entity entity, const unsigned layerIdx );
    void addLayer( const Layer& layer );
    auto getLayers() -> std::vector<Layer>&;
    void setFocusedLayerIdx( const unsigned layerIdx );
    auto getFocusedLayerIdx() -> unsigned;
    auto getLayer( Entity entity ) -> std::optional<unsigned>;
  private:
    std::vector<Layer> _layers{};
    std::unordered_map<Entity, unsigned> _entityToLayerMap{};
    unsigned _focusedLayerIdx{};  // Whatever entity the camera focuses on, all other layers move in parallax wrt that layer.
};

class Scene : public Window {
  public:
    Scene();
    Scene( const int x, const int y, const int w, const int h );
    // TODO make addGrid() or something to replace Scene( const Grid& grid );

    void render() override;
    void onInput( Input& input ) override;
    void setFocus( Entity entity );

  private:
    Grid _grid{};    // This has layers of BGs and FGs.
    Camera _camera{};  // TODO  when we have minimaps, the input needs to go to a window BELOW the top of WindowManager's stack.
    void renderStaticLayer( const Layer& layer, const Rect& camRect );
    Entity _focus{};
};
