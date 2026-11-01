#pragma once
// The part of the 2D and 3D renderers that doesn't care how pixels reach the screen: an SDL3 window, SDL keyboard
// input, and a software framebuffer that windows, text (an 8x16 bitmap font), backgrounds and sprites are drawn into
// in cell coordinates. A cell is CELL_W x CELL_H screen pixels, so an image pixel (2x1 cells) is 16x16.
// Subclasses put the framebuffer on screen: 2D blits it with SDL_Renderer; 3D draws its GL world first and
// composites the framebuffer over it as the 2D overlay.
#include "fixed.h"
#include "v/common/Renderer.h"
#include <cstdint>
#include <memory>
#include <vector>
#include "data/Mem.h"
#include "v/sdl/UiFont.h"
#include "v/sdl/SpriteAtlas.h"

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
    void setMouseCapture( bool on ) override;
    void enableGamepad() override;
    auto quitRequested() const -> bool override { return _quit; }

    auto createWindow( int x, int y, int w, int h ) -> WinHandle override;
    void destroyWindow( WinHandle win ) override;
    void moveWindow( WinHandle win, int x, int y ) override;
    void resizeWindow( WinHandle win, int w, int h ) override;
    auto windowX( WinHandle win ) const -> int override;
    auto windowY( WinHandle win ) const -> int override;
    void clearWindow( WinHandle win ) override;
    void drawBorder( WinHandle win ) override;
    void setPanel( WinHandle win, Panel kind ) override;
    void text( WinHandle win, int y, int x, const std::string& s ) override;
    void setAttr( WinHandle win, int attr, bool on ) override;
    void presentWindow( WinHandle ) override {}
    void endFrame() override;
    bool reuseOverlay() override;
    void setPixelWrites( bool on ) override { _writePixels = on; }
    bool takeOverlayRepaint() override;
    void applyTransition();
    void markOverlayDirty() override;
    void markHudPatch( Entity id ) override;
    bool hudPatch() override;
    void beginHudPatch() override;
    bool hudPatching() override { return _patching; }
    void beginWidget( Entity e ) override;
    void endWidget() override;
    void erasePatchedWidgets() override;

    void preloadImage( const std::shared_ptr<const BImg>& img ) override { _atlas.add( img ); }
    void onSceneUnloaded() override { _atlas.prune(); dropHudTracking(); }

  protected:
    struct Win {
      int x{}, y{}, w{}, h{};  // cells
      int attr{};
      bool cleared{};          // filled this frame (3D: scenes leave their background to the GL world)
      Panel panel{Panel::Dialogue};
    };
    // UI drawn per mode (src/v/2d, src/v/3d): a dialogue/menu window's panel (background, then frame on top of
    // its contents), and the text-row highlight for menus. Scenes get no panel in the pixel modes.
    virtual void drawPanelBody( const Win& w ) = 0;
    virtual void drawPanelFrame( const Win& w ) = 0;
    virtual void drawRowHighlight( const Win& w, int px, int py, int pw, int ph ) = 0;
    // Window pixel rectangle helpers.
    static auto winPx( const Win& w ) -> int { return w.x * CELL_W; }
    static auto winPy( const Win& w ) -> int { return w.y * CELL_H; }
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
    // Like fillRect, but writes argb even when its alpha is 0 (the 3D scene window is cleared to transparent).
    void fillUnchecked( int x0, int y0, int x1, int y1, uint32_t argb );
    void glyph( const Win* win, int px, int py, unsigned char c, uint32_t fg, uint32_t bg );
    // Alpha-blended drawing (straight alpha, so the 3D overlay composites right).
    void clip( const Win* w, int& x0, int& y0, int& x1, int& y1 ) const;  // to w (null = screen) and the screen
    void blendPixel( int x, int y, uint32_t argb, unsigned coverage = 255 );
    void blendRect( const Win* win, int x, int y, int w, int h, uint32_t argb );
    // Rounded rectangle, anti-aliased corners; thickness 0 = filled, else an outline that thick.
    void roundRect( const Win* win, int x, int y, int w, int h, int radius, uint32_t argb, int thickness = 0 );
    // Vertical gradient fill (top color to bottom color), clipped to a rounded rect.
    void gradientRoundRect( const Win* win, int x, int y, int w, int h, int radius, uint32_t top, uint32_t bottom );
    // An image scaled into a rectangle (nearest), its left `fraction` only (bar fills).
    void drawSprite( const Win* win, const BImg& img, int x, int y, int w, int h, float fraction = 1.0f );
    // TrueType UI text at pixel (px, top); returns the advance. Falls back to the bitmap font without fonts/ui.ttf.
    auto drawString( const Win* win, int px, int top, const std::string& s, uint32_t argb, int sizePx, bool bold = false ) -> int;
    auto measureString( const std::string& s, int sizePx, bool bold = false ) -> int;
    static constexpr int UI_TEXT_PX = 15;  // body text size: fits a 16-pixel cell row
    UiFont _font, _fontBold;
    SpriteAtlas _atlas;  // every sprite/tileset image, packed at load (see SpriteAtlas.h)
    void saveScreenshotIfDue();
    auto screenshotDue() const -> bool;  // this frame will be saved (BICYCLE_SCREENSHOT)
    // The finished frame as ARGB, top row first (3D reads it back from GL before swapping).
    virtual void captureFrame( gfx::Pixels& out ) { out = _fb; }
    std::string _gameName;

    SDL_Window* _window{};
    gfx::Pixels _fb;
    int _fbW{};
    int _fbH{};
    // What was drawn into _fb this frame (fills and blends, plus a playing transition). presentFramebuffer and
    // drawOverlay copy _fb to the GPU only when this differs from the last copy, or markOverlayDirty() was called.
    // On 3D, a frame that built the same picture leaves _fb in place, so the next frame does not clear it or fill
    // it again. A HUD patch uploads only the rectangles it erased.
    // _baseSig is everything except HUD widgets; _hudSig is the widgets. A patch whose base still matches did not
    // change dialogue, the scene clear, or the crosshair, so the widget rectangles are the whole update.
    static auto mix( uint64_t s, uint64_t v ) -> uint64_t {
      return s ^ ( v + 0x9e3779b97f4a7c15ull + ( s << 6 ) + ( s >> 2 ) );
    }
    void mixSig( uint64_t v ) {
      _drawSig = mix( _drawSig, v );
      if ( _mixHud ) _hudSig = mix( _hudSig, v );
      else _baseSig = mix( _baseSig, v );
    }
    auto overlayStale() -> bool;
    void noteOverlayUpload();
    void blankFramebuffer();  // the whole _fb to frameClearColor(); a 0 clear is one memset
    auto coverOnCpu() const -> bool;  // a transition that rewrites _fb (not a 3D fade or wipe)
    void dropHudTracking();
    void eraseRect( int x0, int y0, int x1, int y1 );  // zeros, no signature; the following draws mix as usual
    void rememberSpan( Entity e, int x0, int y0, int x1, int y1 );
    void noteHudBox( Entity e, int x0, int y0, int x1, int y1 );
    struct HudSpan { Entity e{}; int x0{}, y0{}, x1{}, y1{}; bool write{}; };
    HudSpan* findSpan( Entity e );
    uint64_t _drawSig{};
    uint64_t _baseSig{};
    uint64_t _hudSig{};
    uint64_t _uploadedSig{};
    uint64_t _uploadedBase{};
    uint64_t _uploadedHud{};
    bool _fbUploaded{};
    bool _writePixels{true};   // false: mix the signature, don't store (the 3D "is this the same picture?" pass)
    bool _mixHud{};            // the draw in progress is a HUD widget
    bool _holdReady{};         // _fb still holds the picture that was uploaded
    bool _overlayRepaint{};    // that pass's signature did not match; draw the overlay for real
    bool _fbBlank{};           // every pixel is 0, so a transparent fill can be skipped
    bool _patching{};          // this frame rewrites HUD rectangles on the held framebuffer
    bool _hudPatch{};          // markHudPatch asked for that, and a full rebuild was not
    bool _hasDamage{};
    int _dmgX0{}, _dmgY0{}, _dmgX1{}, _dmgY1{};
    Entity _hudEntity{};
    std::vector<Entity> _patchIds;
    std::vector<HudSpan> _hudSpans;  // last rectangle each widget actually drew
    std::vector<HudSpan> _noted;     // rectangles measured for this patch
    bool _quit{};
    bool _console{};         // SDL kmsdrm (Linux console): one always-focused window
    int _unreadableInput{};  // console input devices we may not open (not in the input group)
    void reportInput();      // log the video driver and input devices; grab the mouse on the console
    bool _resized{};
    bool _up{};
    long _frame{};
    std::vector<std::unique_ptr<Win>> _wins;

  private:
    void handleEvent( const SDL_Event& e );
    void* _pad{};       // SDL_Gamepad* (enableGamepad)
    bool _padInit{};
};

auto colorToArgb( int color ) -> uint32_t;  // Color (ColorPalette.h) -> ARGB

}  // namespace gfx
