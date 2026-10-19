#pragma once
// What the flat render modes, 2D (Renderer2D : FlatRenderer<PixelRenderer>) and ASCII (AsciiRenderer :
// FlatRenderer<Renderer>), draw the same way:
//   * draw order: a layer's entities in the order they were added, except that each group of overlapping images
//     (include/c/ImageOverlaps.h) is depth-sorted by its members' feet, within the places the group already holds;
//   * text images (ASCII art, in either mode): background layers (spaces see-through, wrapping) and entities (their
//     src rect, cropped to the camera snapped to whole cells), cut into rows of cells for drawCells();
//   * the camera: snapped down to its cell for anything drawn at whole cells.
// Each mode puts cells on screen its own way (drawCells) and draws pixel images itself (2D only).
// A template over the base so 2D keeps PixelRenderer, which it shares with 3D (3D has a depth buffer and none of this).
#include "c/ImageOverlaps.h"
#include "m/Box.h"
#include "m/Rect.h"
#include "m/World.h"
#include "v/common/Image.h"
#include "v/common/Renderer.h"
#include "Constants.h"
#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

namespace gfx {

namespace flat {
// Stable (ties keep their order) and small in code: groups are a handful of sprites.
template <typename T, typename Less>
void insertionSort( std::vector<T>& v, Less less ) {
  for ( size_t i = 1; i < v.size(); ++i ) {
    T x = v[i];
    size_t j = i;
    for ( ; j > 0 && less( x, v[j - 1] ); --j ) v[j] = v[j - 1];
    v[j] = x;
  }
}

// Sorts each group's members in `order` by foot (stable: ties keep their order), each into the slots its group
// holds; entities in no group (groupOf: ImageOverlaps::NONE) keep theirs. `slots` is scratch.
template <typename GroupOf, typename FootOf>
void sortGroups( std::vector<Entity>& order, GroupOf groupOf, FootOf footOf, std::vector<std::pair<Entity, uint32_t>>& slots,
                 std::vector<Entity>& members ) {
  slots.clear();
  for ( uint32_t i = 0; i < order.size(); ++i ) {
    if ( const Entity g = groupOf( order[i] ); g != ImageOverlaps::NONE ) slots.emplace_back( g, i );
  }
  insertionSort( slots, []( const auto& a, const auto& b ) { return a.first < b.first; } );
  for ( size_t s = 0; s < slots.size(); ) {
    size_t t = s;
    members.clear();
    for ( ; t < slots.size() && slots[t].first == slots[s].first; ++t ) members.push_back( order[slots[t].second] );
    insertionSort( members, [&]( Entity a, Entity b ) { return footOf( a ) < footOf( b ); } );
    for ( size_t k = 0; k < members.size(); ++k ) order[slots[s + k].second] = members[k];
    s = t;
  }
}

inline auto wrapIdx( int i, int n ) -> int { return ( ( i % n ) + n ) % n; }
inline auto snapToCell( Box cam ) -> Box {
  constexpr int FRAC = ( 1 << FIXEDPT_DEC_BITS ) - 1;
  cam.pos.x &= ~FRAC;
  cam.pos.y &= ~FRAC;
  return cam;
}
}  // namespace flat

template <class Base>
class FlatRenderer : public Base {
  public:
    auto drawOrder( Entity layer, const std::vector<Entity>& fg ) -> const std::vector<Entity>& override {
      const auto& overlaps = ImageOverlaps::getInstance();
      if ( !overlaps.grouped( layer ) ) return fg;
      _order = fg;
      flat::sortGroups(
          _order, [&]( Entity e ) { return overlaps.group( e ); },
          []( Entity e ) { const Box& b = World::get<Box>( e ); return b.pos.y + static_cast<int>( b.size.h ); },
          _slots, _members );
      return _order;
    }

    // Window cell (r, c) shows image cell (r + offY, c + offX); a text image's runs of non-spaces are drawn.
    void drawLayer( WinHandle win, const Image& img, int offX, int offY, bool wrap, int winW, int winH ) override {
      if ( !img.pixels ) drawTextLayer( win, img, offX, offY, wrap, winW, winH );
      else drawPixelLayer( win, img, offX, offY, wrap, winW, winH );
    }

    void drawEntity( WinHandle win, Entity entity, const Box& camBox ) override {
      if ( !World::get<Image>( entity ).pixels ) drawTextEntity( win, entity, flat::snapToCell( camBox ) );
      else drawPixelEntity( win, entity, camBox );
    }

  protected:
    // Cells of a text image at window cell (y, x) onward: a row of an entity (its spaces included) or a run of a
    // layer (no spaces).
    virtual void drawCells( WinHandle win, int y, int x, std::string_view cells, const Image& img ) = 0;
    // Pixel images (2D; there are none in ASCII).
    virtual void drawPixelLayer( WinHandle, const Image&, int, int, bool, int, int ) {}
    virtual void drawPixelEntity( WinHandle, Entity, const Box& ) {}

  private:
    void drawTextLayer( WinHandle win, const Image& img, int offX, int offY, bool wrap, int w, int h ) {
      const int imgH = img.lineLimits.size();
      const int imgW = img.size.w >> FIXEDPT_DEC_BITS;
      if ( imgH == 0 || imgW == 0 ) return;
      _row.assign( w, ' ' );
      for ( int r = 0; r < h; ++r ) {
        int iy = r + offY;
        if ( wrap ) iy = flat::wrapIdx( iy, imgH );
        else if ( iy < 0 || iy >= imgH ) continue;
        const auto& lims = img.lineLimits[iy];
        for ( int c = 0; c < w; ++c ) {
          const int ix = wrap ? flat::wrapIdx( c + offX, imgW ) : c + offX;
          _row[c] = ( ix >= 0 && ix < lims.len ) ? img.string[lims.start + ix] : ' ';
        }
        for ( int c = 0; c < w; ) {  // spaces are see-through: runs of the rest only
          if ( _row[c] == ' ' ) {
            ++c;
            continue;
          }
          int end = c;
          while ( end < w && _row[end] != ' ' ) ++end;
          drawCells( win, r, c, std::string_view( _row ).substr( c, end - c ), img );
          c = end;
        }
      }
    }

    // The part of the entity's image (or its src rect: an animation frame) that the camera, at a whole cell, sees.
    void drawTextEntity( WinHandle win, Entity entity, const Box& camBox ) {
      const auto& entityBox = World::get<Box>( entity );
      const auto& img = World::get<Image>( entity );
      const auto& src = World::get<Rect>( entity );
      const bool hasSrc = src.w > 0;
      const auto cropped = camBox.crop( entityBox ) >> FIXEDPT_DEC_BITS;  // the part of entityBox in view
      const auto rel = ( entityBox.pos - camBox.pos ) >> FIXEDPT_DEC_BITS;
      unsigned iy = std::abs( std::min( 0, rel.y ) ) + ( hasSrc ? src.y : 0 );
      const int clipLeft = std::abs( std::min( rel.x, 0 ) );
      const int y0 = cropped.pos.y - ( camBox.pos.y >> FIXEDPT_DEC_BITS );
      const int x0 = cropped.pos.x - ( camBox.pos.x >> FIXEDPT_DEC_BITS );
      for ( int r = 0; r < static_cast<int>( cropped.size.h ) && iy < img.lineLimits.size(); ++r, ++iy ) {
        const auto& lims = img.lineLimits[iy];
        const int rowStart = hasSrc ? src.x : 0;
        const int rowLen = hasSrc ? std::min( src.w, lims.len - src.x ) : lims.len;
        const int len = std::min( static_cast<int>( cropped.size.w ), rowLen - clipLeft );
        if ( len <= 0 ) continue;
        drawCells( win, y0 + r, x0, std::string_view( img.string ).substr( lims.start + rowStart + clipLeft, len ), img );
      }
    }

    std::vector<Entity> _order, _members;
    std::vector<std::pair<Entity, uint32_t>> _slots;
    std::string _row;
};

}  // namespace gfx
