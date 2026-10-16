// 2D mode: the pixel framebuffer (see PixelRenderer) shown through an SDL_Renderer streaming texture.
#include "v/sdl/PixelRenderer.h"
#include <SDL3/SDL.h>
#include <cstdio>

namespace gfx {

class Renderer2D final : public PixelRenderer {
  public:
    auto mode() const -> Mode override { return Mode::TwoD; }
  protected:
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
