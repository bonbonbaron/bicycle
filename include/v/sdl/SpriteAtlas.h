#pragma once
// Every sprite and tileset image of the pixel modes packed into one atlas (jollybean's planner, data/Atlas.h),
// as ARGB pixels in the GRAPHIC pool. Images are registered as they load (Renderer::preloadImage); the atlas is
// re-planned once, lazily, the next time something draws after new images arrived, i.e. at load time, not per
// frame. 2D reads sprite pixels from it; 3D uploads it as one texture and batches every billboard into one draw.
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include "data/Mem.h"

struct BImg;

namespace gfx {

class SpriteAtlas {
  public:
    struct Entry { int x{}, y{}, w{}, h{}; };
    static constexpr uint32_t PAD = 1;  // transparent gutter so GPU sampling never bleeds between images

    void add( const std::shared_ptr<const BImg>& img );           // no-op if already in
    auto entry( const std::shared_ptr<const BImg>& img ) -> Entry;  // adds it if needed; rebuilds if dirty
    void rebuildIfDirty();
    auto pixels() const -> const mem::Vector<uint32_t, mem::GRAPHIC>& { return _px; }  // ARGB, top row first
    auto width() const -> int { return _w; }
    auto height() const -> int { return _h; }
    auto version() const -> uint32_t { return _version; }  // bumps on every rebuild
    auto images() const -> size_t { return _imgs.size(); }
    auto at( int x, int y ) const -> uint32_t { return _px[static_cast<size_t>( y ) * _w + x]; }

  private:
    std::vector<std::shared_ptr<const BImg>> _imgs;  // holding them keeps their addresses unique
    std::unordered_map<const BImg*, size_t> _idx;
    std::vector<Entry> _entries;
    mem::Vector<uint32_t, mem::GRAPHIC> _px;
    int _w{}, _h{};
    bool _dirty{};
    uint32_t _version{};
};

}  // namespace gfx
