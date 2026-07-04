#pragma once
#include "m/Entity.h"
#include <vector>
#include <list>

// Camera can be moved in update().
class Camera {
  public:
    Camera();
    Camera( const int x, const int y, const int margin );
    void pan( const int dx, const int dy );
    auto canSee( const Entity entity ) const -> bool;
    void focusOn( const Entity entity );
    auto getFocus() const -> Entity;
    void setDims( const unsigned h, const unsigned w);
    void setLims( const int y, const int x);
    // Margins tell us how close the focused-on entity can come to the 
    // FOV's edge before the camera should be told to move.
    auto getLxMargin() const -> int;  // low x margin
    auto getLyMargin() const -> int;  // low y margin
    auto getHxMargin() const -> int;  // high x margin
    auto getHyMargin() const -> int;  // high y margin
    void followFocus();   // tracks the focus when it moves out of the center's margin
    auto getId() const -> Entity;
  private:
    Entity _id{};  // the camera itself is an entity with an entity ID. This helps motion and collision systems track it.
    int _maxX{};
    int _maxY{};
    int _margin{};  // distance focused-on Entity can move from center before camera follows it
    Entity _focus{};  // input is forwarded to this guy
    std::vector<std::list<Entity>> onscreenEntities;  // each list element corresponds to a layer in the scene
};

