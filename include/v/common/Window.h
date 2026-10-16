#pragma once
#include <string>
#include "v/common/ColorPalette.h"
#include "v/common/Renderer.h"
#include "c/Input.h"
#include "m/Entity.h"

static constexpr int WINDOW_PADDING{2};

// A rectangle of cells on screen (a curses window in ASCII mode). Drawing goes through gfx::renderer().
class Window {
  public:
    // i know this doesn't align with ncurses' paramter order, but I like my order better :)
    Window();
    Window( const int w, const int h );
    Window( const int x, const int y, const int w, const int h );
    virtual ~Window();
    void create();
    void show();
    void hide();
    void clear();
    virtual void render() = 0;
    virtual void onInput( Input& input ) = 0;
    // The screen changed size (gfx::cols() and gfx::lines() are already updated). By default the window keeps its
    // size and position but is pulled back inside the screen.
    virtual void onTermResize();
    virtual auto panelKind() const -> gfx::Panel { return gfx::Panel::Dialogue; }  // the screen changed size (gfx::cols() and gfx::lines() are already updated)
    void repaint();

    auto getX() const -> int;
    auto getY() const -> int;
    auto getWidth() const -> unsigned;
    auto getHeight() const -> unsigned;

    void setX( const int x );
    void setY( const int y );
    void setWidth( const int w );
    void setHeight( const int h );
    // Moves and resizes in one step, in the order curses accepts (a window may never stick out of the screen).
    void place( int x, int y, int w, int h );
    auto requestedX() const -> int { return _x; }  // where the window asked to be (getX() is where curses put it)
    auto requestedY() const -> int { return _y; }

    void setBorder( const bool showBorder );
    auto isBorderShowing() const -> bool;
    void mvprint( const int y, const int x, const std::string& s ) const;

    auto getContext() const -> Entity; // window's an entity now
    auto getId() const -> Entity;
    void setAttr( const int attr );    // gfx::Attr flags
    void unsetAttr( const int attr );
    auto getHandle() const -> gfx::WinHandle { return _win; }

  private:
    gfx::WinHandle _win{};
    bool _showBorder{true};
    std::string name;
    int _w{};
    int _h{};
    int _x{};
    int _y{};
    const Entity _id{};
    const Entity _context{};
};
