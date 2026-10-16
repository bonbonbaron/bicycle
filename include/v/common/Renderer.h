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
#include <memory>
#include <string>
#include "m/Entity.h"

struct Image;
struct Box;
struct BImg;

namespace gfx {

enum class Mode { Ascii, TwoD, ThreeD };

enum Attr : int {
  ATTR_STANDOUT = 1,  // highlighted text (menu selection)
  ATTR_INVISIBLE = 2,
};

using WinHandle = void*;  // a curses WINDOW* in ASCII, a rectangle of cells elsewhere

// What a window is, so each mode can decorate it its own way: ASCII draws a box of line characters around every
// kind; the pixel modes draw dialogue and menu windows as graphical panels and leave a scene's view undecorated.
enum class Panel { Scene, Dialogue, Menu };

// A meter (health, stamina, progress). ASCII draws "HP [#######---] 7/10"; the pixel modes draw a graphical bar
// (frame + fill colored by fraction), or the given sprites: frame drawn as is, fill cropped to the fraction.
struct BarSpec {
  std::string label;
  int curr{}, max{1};
  int len{};  // in cells (ASCII: segments)
  std::shared_ptr<const BImg> frameSprite, fillSprite;
};

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
    // The screen is below the smallest usable size: WindowManager skips the windows and the renderer shows a
    // "too small" notice until it grows again (ASCII; pixel windows scale instead).
    virtual auto tooSmall() const -> bool { return false; }

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
    virtual void drawBorder( WinHandle win ) = 0;  // the window's frame (see Panel)
    virtual void setPanel( WinHandle win, Panel kind ) {}  // set before the window clears (Window::clear)
    // UI widgets, each mode its own way (cells are the window's).
    virtual void drawBar( WinHandle win, int y, int x, const BarSpec& bar ) = 0;
    // A menu's selection marker in front of row y; ASCII prints the menu's cursor string.
    virtual void drawMenuCursor( WinHandle win, int y, int x, const std::string& asciiCursor ) = 0;
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
    // The next drawLayer's offsets are whole cells; this is the remainder in 1/256 cell (0..255 each) for
    // renderers that can draw between cells (pixel modes shift by it in screen pixels; ASCII snaps).
    virtual void setLayerSubcell( int fx, int fy ) {}
    virtual void endScene( WinHandle win ) {}
    // 3D mode: an entity drawn as a mesh instead of its sprite, and static scenery (other modes ignore both).
    // Paths are asset paths of .bmesh files (see include/data/Mesh.h).
    virtual auto setModel( Entity entity, const std::string& bmeshPath ) -> bool { return false; }
    virtual auto addStaticModel( const std::string& bmeshPath ) -> bool { return false; }
    // 3D mode: which action (e.g. "walk", "idle") a rigged model plays.
    virtual void setModelAction( Entity entity, const std::string& action ) {}
    // A sprite/tileset image just loaded: the pixel modes pack it into their texture atlas (ASCII ignores it).
    virtual void preloadImage( const std::shared_ptr<const BImg>& img ) {}
    // Real-time input (include/c/InputState.h): mouse look and gamepads (pixel modes; ASCII ignores them).
    virtual void setMouseCapture( bool on ) {}
    virtual void enableGamepad() {}
    // Scene changes: forget a despawned entity's per-entity state (3D models), then, once the old scene's objects
    // are gone, drop what only it used (atlas images, scenery, cached meshes) so its memory can be freed.
    virtual void forgetEntity( Entity entity ) {}
    virtual void onSceneUnloaded() {}
    // Scene transitions, drawn over everything at the end of each frame: kind 0 none, 1 fade (to black), 2 wipe
    // (left to right), 3 dissolve (cells drop out in a fixed random order; ASCII's own); t from 0 (clear) to 1
    // (covered). Every mode accepts every kind and draws the closest it has.
    virtual void setTransition( int kind, float t ) { _transKind = kind; _transT = t; }
  protected:
    int _transKind{};
    float _transT{};
  public:
};

auto renderer() -> Renderer&;  // the build's renderer (BICYCLE_RENDER)
inline auto cols() -> int { return renderer().cols(); }
inline auto lines() -> int { return renderer().lines(); }

}  // namespace gfx
