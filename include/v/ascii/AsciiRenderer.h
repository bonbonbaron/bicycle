#pragma once
#include "v/common/Renderer.h"

namespace gfx {

// curses: ncurses on Linux and macOS, PDCurses on Windows (include/curses_compat.h).
class AsciiRenderer : public Renderer {
  public:
    auto mode() const -> Mode override { return Mode::Ascii; }
    void init( const std::string& gameName ) override;
    void shutdown() override;
    auto cols() const -> int override;
    auto lines() const -> int override;
    auto checkResize() -> bool override;
    void pollInput() override;
    auto createWindow( int x, int y, int w, int h ) -> WinHandle override;
    void destroyWindow( WinHandle win ) override;
    void moveWindow( WinHandle win, int x, int y ) override;
    void resizeWindow( WinHandle win, int w, int h ) override;
    auto windowX( WinHandle win ) const -> int override;
    auto windowY( WinHandle win ) const -> int override;
    void clearWindow( WinHandle win ) override;
    void drawBorder( WinHandle win ) override;
    void drawBar( WinHandle win, int y, int x, const BarSpec& bar ) override;
    void drawMenuCursor( WinHandle win, int y, int x, const std::string& asciiCursor ) override;
    void text( WinHandle win, int y, int x, const std::string& s ) override;
    void setAttr( WinHandle win, int attr, bool on ) override;
    void presentWindow( WinHandle win ) override;
    void endFrame() override;
    void drawLayer( WinHandle win, const Image& img, int offX, int offY, bool wrap, int winW, int winH ) override;
    void drawEntity( WinHandle win, Entity entity, const Box& camBox ) override;
  private:
    bool _up{};
    bool _inFrame{};
    long long _frameStart{}, _renderNs{}, _maxNs{};
    long _frames{};
};

}  // namespace gfx
