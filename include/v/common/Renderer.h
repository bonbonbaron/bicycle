#pragma once
// The one interface between the engine and whatever draws it. The engine (Scene, Camera, Window, Dialogue, Bar,
// TextMenu, WindowManager, input) only talks to gfx::renderer(); each render mode implements it:
//   ASCII  src/v/ascii  curses (ncurses / PDCurses) in a terminal          (BICYCLE_RENDER=ASCII)
//   2D     src/v/2d     SDL3 pixel graphics                                 (BICYCLE_RENDER=2D)
//   3D     src/v/3d     SDL3 + OpenGL 3.3: meshes for entities that have one, (BICYCLE_RENDER=3D)
//                       2D sprites as billboards and the 2D overlay otherwise
// Everything is laid out in character cells, the engine's world unit, in every mode: an image pixel is 2 cells
// wide and 1 cell tall (the ASCII exporter's 2-chars-per-pixel scheme), so game logic, positions, collision and
// camera are the same in all modes. Pixel modes just draw a cell as CELL_W x CELL_H screen pixels.
#include <string>
#include "m/Entity.h"

struct Image;
struct Box;

namespace gfx {

enum class Mode { Ascii, TwoD, ThreeD };

enum Attr : int {
  ATTR_STANDOUT = 1,  // highlighted text (menu selection)
  ATTR_INVISIBLE = 2,
};

using WinHandle = void*;  // a curses WINDOW* in ASCII, a rectangle of cells elsewhere

class Renderer {
  public:
    virtual ~Renderer() = default;
    virtual auto mode() const -> Mode = 0;
    virtual void init( const std::string& gameName ) = 0;
    virtual void shutdown() = 0;  // restores the terminal / closes the window; safe to call twice

    // Screen size in cells.
    virtual auto cols() const -> int = 0;
    virtual auto lines() const -> int = 0;
    // Whether the screen changed size since the last call (cols()/lines() are already updated).
    virtual auto checkResize() -> bool = 0;

    // Reads pending key presses and hands them to the Trigger system.
    virtual void pollInput() = 0;
    // The user closed the window (pixel modes).
    virtual auto quitRequested() const -> bool { return false; }

    // Windows: rectangles of cells drawn in stack order, each frame.
    virtual auto createWindow( int x, int y, int w, int h ) -> WinHandle = 0;
    virtual void destroyWindow( WinHandle win ) = 0;
    virtual void moveWindow( WinHandle win, int x, int y ) = 0;
    virtual void resizeWindow( WinHandle win, int w, int h ) = 0;
    virtual auto windowX( WinHandle win ) const -> int = 0;
    virtual auto windowY( WinHandle win ) const -> int = 0;
    virtual void clearWindow( WinHandle win ) = 0;
    virtual void drawBorder( WinHandle win ) = 0;
    virtual void text( WinHandle win, int y, int x, const std::string& s ) = 0;
    virtual void setAttr( WinHandle win, int attr, bool on ) = 0;
    virtual void presentWindow( WinHandle win ) = 0;  // the window is done for this frame
    virtual void endFrame() = 0;                     // show everything presented this frame

    // Scenes. camBox and entity boxes are in fixed-point cells (see Constants.h).
    // A background layer: window cell (r, c) shows image cell (r + offY, c + offX), wrapping if asked.
    virtual void drawLayer( WinHandle win, const Image& img, int offX, int offY, bool wrap, int winW, int winH ) = 0;
    // One entity seen by a camera whose box is camBox; the entity overlaps it.
    virtual void drawEntity( WinHandle win, Entity entity, const Box& camBox ) = 0;
    // Brackets one scene's layers and entities (the 3D renderer draws its world here).
    virtual void beginScene( WinHandle win, const Box& camBox ) {}
    virtual void endScene( WinHandle win ) {}
};

auto renderer() -> Renderer&;  // the build's renderer (BICYCLE_RENDER)
inline auto cols() -> int { return renderer().cols(); }
inline auto lines() -> int { return renderer().lines(); }

}  // namespace gfx
