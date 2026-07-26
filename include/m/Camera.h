#pragma once
#include "m/Entity.h"
#include <vector>
#include <list>
#include "v/Window.h"
#include "m/Box.h"

class Camera {
  public:
    Camera();
    Camera( const int x, const int y, const int margin );
    void pan( const int dx, const int dy );
    auto canSee( const Entity entity ) const -> bool;

    inline auto _canSee( const Box& camBox, const Box& entityBox ) const -> bool {
      return 
        entityBox.pos.x >= static_cast<decltype(Position::x)>( camBox.pos.x ) && 
        entityBox.pos.x <= static_cast<decltype(Position::x)>( camBox.pos.x + camBox.size.w ) &&
        entityBox.pos.y >= static_cast<decltype(Position::y)>( camBox.pos.y ) && 
        entityBox.pos.y <= static_cast<decltype(Position::y)>( camBox.pos.y + camBox.size.h ); 
    }
    void focusOn( const Entity entity );
    auto getFocus() const -> Entity;
    void setDims( const unsigned h, const unsigned w);
    void setLims( const int y, const int x);
    // Margins tell us how close the focused-on entity can come to the 
    // FOV's edge before the camera should be told to move.
    void setMargin( const int margin );
    auto getLxMargin() const -> int;  // low x margin
    auto getLyMargin() const -> int;  // low y margin
    auto getHxMargin() const -> int;  // high x margin
    auto getHyMargin() const -> int;  // high y margin
    void followFocus();   // tracks the focus when it moves out of the center's margin
    auto getId() const -> Entity;
    /* I choose "draw" instead of "render" since rendering doesn't
     * actually happen till the last part of the game loop. I know,
     * kind of a weird word choice for a camera. */
    void draw( const Entity entity, Window& tgt );
  private:
    Entity _id{};  // the camera itself is an entity with an entity ID. This helps motion and collision systems track it.
    int _maxX{};
    int _maxY{};
    int _margin{};  // distance focused-on Entity can move from center before camera follows it
    Entity _focus{};  // input is forwarded to this guy
    std::vector<std::list<Entity>> onscreenEntities;  // each list element corresponds to a layer in the scene
};

