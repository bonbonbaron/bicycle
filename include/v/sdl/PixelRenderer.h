#pragma once
// The part of the 2D and 3D renderers that doesn't care how pixels reach the screen: an SDL3 window, SDL keyboard
// input, and a software framebuffer that windows, text (an 8x16 bitmap font), backgrounds and sprites are drawn into
// in cell coordinates. A cell is CELL_W x CELL_H screen pixels, so an image pixel (2x1 cells) is 16x16.
// Subclasses put the framebuffer on screen: 2D blits it with SDL_Renderer; 3D draws its GL world first and
// composites the framebuffer over it as the 2D overlay.
#include "v/common/Renderer.h"
#include <cstdint>
#include <memory>
#include <vector>
#include "data/Mem.h"

struct SDL_Window;
union SDL_Event;

namespace gfx {

// Framebuffers and GL staging live in the GRAPHIC pool.
using Pixels = mem::Vector<uint32_t, mem::GRAPHIC>;

class PixelRenderer : public Renderer {
  public:
    static constexpr int CELL_W = 8;
    static constexpr int CELL_H = 16;

    void init( const std::string& gameName ) override;
    void shutdown() override;
    auto cols() const -> int override { return _fbW / CELL_W; }
    auto lines() const -> int override { return _fbH / CELL_H; }
    auto checkResize() -> bool override;
    void pollInput() override;
    auto quitRequested() const -> bool override { return _quit; }

    auto createWindow( int x, int y, int w, int h ) -> WinHandle override;
    void destroyWindow( WinHandle win ) override;
    void moveWindow( WinHandle win, int x, int y ) override;
    void resizeWindow( WinHandle win, int w, int h ) override;
    auto windowX( WinHandle win ) const -> int override;
    auto windowY( WinHandle win ) const -> int override;
    void clearWindow( WinHandle win ) override;
    void drawBorder( WinHandle win ) override;
    void text( WinHandle win, int y, int x, const std::string& s ) override;
    void setAttr( WinHandle win, int attr, bool on ) override;
    void presentWindow( WinHandle ) override {}
    void endFrame() override;

    void drawLayer( WinHandle win, const Image& img, int offX, int offY, bool wrap, int winW, int winH ) override;
    void drawEntity( WinHandle win, Entity entity, const Box& camBox ) override;

  protected:
    struct Win {
      int x{}, y{}, w{}, h{};  // cells
      int attr{};
      bool cleared{};          // filled this frame (3D: scenes leave their background to the GL world)
    };
    // Window flags for SDL_CreateWindow (3D adds SDL_WINDOW_OPENGL) and hooks for the subclass.
    virtual auto windowFlags() const -> uint64_t;
    virtual void beforeWindowCreated() {}   // e.g. GL attributes
    virtual void onWindowCreated() = 0;
    virtual void onWindowDestroyed() = 0;
    virtual void presentFramebuffer() = 0;  // show _fb (ARGB8888, _fbW x _fbH)
    virtual void onResized() {}
    // Clears the frame before windows draw; 3D clears to transparent so the GL world shows through.
    virtual auto frameClearColor() const -> uint32_t { return 0xFF000000u; }

    // Pixel helpers, in screen pixels, clipped to win (null = whole screen). ARGB; alpha 0 skips.
    void fillRect( const Win* win, int x, int y, int w, int h, uint32_t argb );
    void glyph( const Win* win, int px, int py, unsigned char c, uint32_t fg, uint32_t bg );
    void drawTextImage( const Win* win, const Image& img, int srcX, int srcY, int srcW, int srcH, int dstX, int dstY );
    void saveScreenshotIfDue();
    auto screenshotDue() const -> bool;  // this frame will be saved (BICYCLE_SCREENSHOT)
    // The finished frame as ARGB, top row first (3D reads it back from GL before swapping).
    virtual void captureFrame( gfx::Pixels& out ) { out = _fb; }
    std::string _gameName;

    SDL_Window* _window{};
    gfx::Pixels _fb;
    int _fbW{};
    int _fbH{};
    bool _quit{};
    bool _resized{};
    bool _up{};
    long _frame{};
    std::vector<std::unique_ptr<Win>> _wins;

  private:
    void handleEvent( const SDL_Event& e );
};

auto colorToArgb( int color ) -> uint32_t;  // Color (ColorPalette.h) -> ARGB

}  // namespace gfx
