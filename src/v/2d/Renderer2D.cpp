// 2D mode: the pixel framebuffer (see PixelRenderer) shown through an SDL_Renderer streaming texture.
#include "v/sdl/PixelRenderer.h"
#include "data/BImg.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <string>

namespace gfx {

class Renderer2D final : public PixelRenderer {
  public:
    auto mode() const -> Mode override { return Mode::TwoD; }

    // 2D UI: flat, crisp, pixel-art flavored. A health bar is a dark frame with a segmented fill whose color goes
    // green -> yellow -> red with the fraction, a label in front and "curr/max" after it.
    void drawBar( WinHandle win, int y, int x, const BarSpec& bar ) override {
      const Win* w = static_cast<Win*>( win );
      if ( !w || ( w->attr & ATTR_INVISIBLE ) ) return;
      int px = ( w->x + x ) * CELL_W + 4;
      const int top = ( w->y + y ) * CELL_H;
      const float frac = static_cast<float>( bar.curr ) / std::max( 1, bar.max );
      if ( !bar.label.empty() ) {
        px += drawString( w, px + 1, top + 1, bar.label, 0xC0000000u, UI_TEXT_PX, true );  // drop shadow
        drawString( w, px - measureString( bar.label, UI_TEXT_PX, true ), top, bar.label, 0xFFFFF4D8u, UI_TEXT_PX, true );
        px += 6;
      }
      const int bw = std::max( 16, bar.len * CELL_W * 2 ), bh = 14, by = top + 1;
      if ( bar.frameSprite || bar.fillSprite ) {
        if ( bar.fillSprite ) drawSprite( w, *bar.fillSprite, px, by, bw, bh, frac );
        if ( bar.frameSprite ) drawSprite( w, *bar.frameSprite, px, by, bw, bh );
      }
      else {
        fillRect( w, px - 2, by - 2, bw + 4, bh + 4, 0xFF101018u );  // outer frame
        fillRect( w, px - 1, by - 1, bw + 2, bh + 2, 0xFFE8D8A8u );  // parchment rim
        fillRect( w, px, by, bw, bh, 0xFF2A1E22u );                  // empty well
        const uint32_t c = frac > 0.5f ? 0xFF48C048u : frac > 0.25f ? 0xFFE8C030u : 0xFFE04030u;
        const uint32_t hi = frac > 0.5f ? 0xFF90F080u : frac > 0.25f ? 0xFFFFE890u : 0xFFFF9070u;
        const int fw = bar.curr > 0 ? std::max( 2, static_cast<int>( bw * frac + 0.5f ) ) : 0;
        fillRect( w, px, by, fw, bh, c );
        fillRect( w, px, by, fw, 3, hi );                        // highlight band
        blendRect( w, px, by + bh - 3, fw, 3, 0x50000000u );
        const int seg = std::max( 1, bar.len );                  // segment notches
        for ( int i = 1; i < seg; ++i ) blendRect( w, px + bw * i / seg, by, 1, bh, 0x60000000u );
      }
      px += bw + 8;
      const std::string nums = std::to_string( bar.curr ) + "/" + std::to_string( bar.max );
      drawString( w, px + 1, top + 1, nums, 0xC0000000u, UI_TEXT_PX );
      drawString( w, px, top, nums, 0xFFFFF4D8u, UI_TEXT_PX );
    }

    // A right-pointing gold arrow, pixel-art style.
    void drawMenuCursor( WinHandle win, int y, int x, const std::string& ) override {
      const Win* w = static_cast<Win*>( win );
      if ( !w || ( w->attr & ATTR_INVISIBLE ) ) return;
      const int px = ( w->x + x ) * CELL_W + 4, py = ( w->y + y ) * CELL_H + 2;
      for ( int r = 0; r < 12; ++r ) {
        const int len = 6 - std::abs( r - 6 ) + ( r < 6 ? 1 : 0 );
        fillRect( w, px, py + r, len + 1, 1, 0xFFF0C040u );
      }
    }

  protected:
    void drawPanelBody( const Win& w ) override {
      const int x = winPx( w ), y = winPy( w ), pw = w.w * CELL_W, ph = w.h * CELL_H;
      blendRect( nullptr, x + 4, y + 4, pw, ph, 0x70000000u );  // hard pixel shadow
      fillRect( &w, x, y, pw, ph, 0xFF1C2238u );
      fillRect( &w, x, y, pw, 3, 0xFF2C3458u );
    }
    void drawPanelFrame( const Win& w ) override {
      const int x = winPx( w ), y = winPy( w ), pw = w.w * CELL_W, ph = w.h * CELL_H;
      // Two-tone pixel frame: dark outer line, parchment rim, dark inner line.
      auto frame = [&]( int i, int t, uint32_t c ) {
        fillRect( &w, x + i, y + i, pw - 2 * i, t, c );
        fillRect( &w, x + i, y + ph - i - t, pw - 2 * i, t, c );
        fillRect( &w, x + i, y + i, t, ph - 2 * i, c );
        fillRect( &w, x + pw - i - t, y + i, t, ph - 2 * i, c );
      };
      frame( 0, 1, 0xFF0C0C14u );
      frame( 1, 2, 0xFFE8D8A8u );
      frame( 3, 1, 0xFF6A5A40u );
    }
    void drawRowHighlight( const Win& w, int px, int py, int pw, int ph ) override {
      fillRect( &w, px, py + 1, pw, ph - 2, 0xFF3A5A9Au );
      fillRect( &w, px, py + 1, pw, 1, 0xFF6A8ACAu );
    }

    void onWindowCreated() override {
      _sdl = SDL_CreateRenderer( _window, nullptr );
      if ( !_sdl ) {
        std::fprintf( stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError() );
        return;
      }
      SDL_SetRenderVSync( _sdl, 0 );  // bicycle paces frames itself (Timer::sleepFrame)
      makeTexture();
    }
    void onWindowDestroyed() override {
      if ( _tex ) SDL_DestroyTexture( _tex );
      if ( _sdl ) SDL_DestroyRenderer( _sdl );
      _tex = nullptr;
      _sdl = nullptr;
    }
    void onResized() override { makeTexture(); }
    void presentFramebuffer() override {
      if ( !_sdl || !_tex ) return;
      SDL_UpdateTexture( _tex, nullptr, _fb.data(), _fbW * 4 );
      SDL_RenderClear( _sdl );
      SDL_RenderTexture( _sdl, _tex, nullptr, nullptr );
      SDL_RenderPresent( _sdl );
    }
  private:
    void makeTexture() {
      if ( _tex ) SDL_DestroyTexture( _tex );
      _tex = SDL_CreateTexture( _sdl, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, _fbW, _fbH );
      SDL_SetTextureScaleMode( _tex, SDL_SCALEMODE_NEAREST );
    }
    SDL_Renderer* _sdl{};
    SDL_Texture* _tex{};
};

auto renderer() -> Renderer& {
  static Renderer2D r;
  return r;
}

}  // namespace gfx
