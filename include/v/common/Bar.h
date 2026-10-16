#pragma once
#include <memory>
#include <string>
#include "v/common/Window.h"

// A labeled meter (health, stamina, a progress bar...) drawn glued to a window, i.e. in screen cells. Each render
// mode draws it its own way (gfx::Renderer::drawBar): ASCII as text,
//   HP [#######---] 7/10
// the pixel modes as a graphical bar, optionally from sprites. The ASCII fill is curr/max of len segments,
// rounded up so any value above 0 shows at least one.
class Bar {
  public:
    Bar( const std::string& label, const int max, const int len, const int x, const int y );
    void setValue( const int val );
    auto getValue() const -> int;
    void setPos( const int x, const int y );
    auto filledLength() const -> int;
    // Pixel modes: draw the bar from images (frame as is, fill cropped to the fraction); null = drawn bar.
    void setSprites( std::shared_ptr<const BImg> frame, std::shared_ptr<const BImg> fill );
    void draw( const Window& tgt ) const;
  private:
    gfx::BarSpec _spec{};
    int _x{};
    int _y{};
};
