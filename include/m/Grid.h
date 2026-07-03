#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "m/Entity.h"
#include "m/Position.h"
#include "m/Size.h"
#include "m/World.h"

struct Layer {
  enum Type { STATIC, PARALLAX, AUTOLOOP };

  Layer() : id( newEntityId() ) {}
  Layer( const std::string bgStr = "", const std::string& bgCollStr = "", Layer::Type type = Layer::Type::STATIC ) 
    : id( newEntityId() ) ,
      bgStr( bgStr ),
      bgCollStr( bgCollStr ),
      type( type ) {
        // TODO register size to World
        unsigned maxLineWidth{}, currNumLines{};
        signed lineWidth{};
        bool continueLooking{ true };
        const char ENDROW{ '\n' };
        for ( unsigned currStart{}; continueLooking; ++currNumLines ) {
          lineWidth = bgStr.find( "\n", currStart, 1 ) - currStart;
          if ( lineWidth < 0 ) {
            lineWidth = bgStr.size() - currStart;
            continueLooking = false;
          }
          maxLineWidth = std::max( maxLineWidth, static_cast<unsigned>( lineWidth ) );
          currStart += lineWidth + 1;  // "+1" includes the newline character.
        }
        this->bgStr.erase(std::remove(this->bgStr.begin(), this->bgStr.end(), ENDROW), this->bgStr.end());
        this->bgCollStr.erase(std::remove(this->bgCollStr.begin(), this->bgCollStr.end(), ENDROW), this->bgCollStr.end());
        World::set<Size>( id, { currNumLines, maxLineWidth } );
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
