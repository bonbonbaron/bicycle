#pragma once
// TrueType text for the pixel modes' UI (dialogue, menus, HUD labels): fonts/ui.ttf rasterized with stb_truetype,
// anti-aliased, proportional, glyphs cached per size. The font file and glyph bitmaps live in the GRAPHIC pool.
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include "data/Mem.h"

namespace gfx {

class UiFont {
  public:
    struct Glyph {
      int w{}, h{}, xoff{}, yoff{};  // bitmap size and offset from the pen (yoff from the baseline)
      float advance{};
      mem::Vector<uint8_t, mem::GRAPHIC> coverage;  // w * h, 0..255
    };
    UiFont();
    ~UiFont();
    auto load( const std::string& path ) -> bool;
    auto loaded() const -> bool { return _ok; }
    auto glyph( uint32_t codepoint, int px ) -> const Glyph&;
    auto ascent( int px ) const -> float;
    auto lineHeight( int px ) const -> float;
    auto measure( const std::string& utf8, int px ) -> float;
    auto kern( uint32_t a, uint32_t b, int px ) const -> float;
  private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
    mem::Blob _bytes;
    bool _ok{};
    std::map<uint64_t, Glyph> _cache;
};

// UTF-8 decoding for the UI text (invalid bytes come out as '?').
auto nextCodepoint( const std::string& s, size_t& i ) -> uint32_t;

}  // namespace gfx
