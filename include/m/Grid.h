#pragma once
#include <string>
#include <vector>
#include "m/Entity.h"
#include "m/Position.h"
#include "m/Size.h"

struct Layer {
  enum Type { STATIC, PARALLAX, AUTOLOOP };

  Layer() : id( newEntityId() ) {}
  Layer( const std::string bgStr = "", const std::string& bgCollStr = "", Layer::Type type = Layer::Type::STATIC ) 
    : id( newEntityId() ) ,
      bgStr( bgStr ),
      bgCollStr( bgCollStr ),
      type( type ) {
        // TODO register size to World
  }

  Entity id{};
  std::string bgStr;
  std::string bgCollStr;
  // Things Layer needs to have in World:
  //   * entity ID
  //   * image
  //   * position (if PARALLAX or AUTOLOOP)
  //   * velocity (if AUTOLOOP)
  // Backgrounds can have positions other than (0,0) for parallax or auto-looping
  // That implies they need to give a Position and potentially a Velocity to World.
  // They may need to pass an Image in too.
  Type type{};
  std::vector<Entity> fg{};  // entities in the foreground
  // TODO some timer tracker to know which BG tile to animate next
};

class Grid {
  public:
    void addEntity( const Entity entity, const unsigned layerIdx );
    void moveEntity( const Entity entity, const unsigned layerIdx );
    void addLayer( const Layer& layer );
    auto getLayers() -> std::vector<Layer>&;
  private:
    std::vector<Layer> _layers{};
    unsigned focusedLayerIdx{};  // Whatever entity the camera focuses on, all other layers move in parallax wrt that layer.
};
