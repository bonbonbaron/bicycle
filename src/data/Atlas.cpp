// Port of jollybean's texture atlas planner (src/x/xRender.c). Names and control flow follow the original.
#include "data/Atlas.h"
#include <cassert>

namespace atlas {

namespace {

// bt.h
inline void _btHeaderClr( BtElHeader& h ) { h = BtElHeader{}; h.orphan = false; }
inline auto _btIsAnOrphan( const BtElHeader& h ) -> bool { return h.orphan; }
inline void _btLinkNodes( BtElHeader& parentHdr, BtElHeader& childHdr, Key parentIdx, Key childIdx, Child child ) {
  parentHdr.childA[child] = childIdx;
  _btHeaderClr( childHdr );
  childHdr.parent = parentIdx;
}

// "can"/"should" bits for growing the atlas (xRender.h)
constexpr uint32_t CAN_RIGHT = 0x1, SHOULD_RIGHT = 0x3, CAN_DOWN = 0x4, SHOULD_DOWN = 0xc;
constexpr uint32_t SHOULD_RIGHT_CAN_DOWN = SHOULD_RIGHT | CAN_DOWN, CAN_RIGHT_SHOULD_DOWN = SHOULD_DOWN | CAN_RIGHT;
constexpr uint32_t SHOULD_RIGHT_DOWN = SHOULD_RIGHT | SHOULD_DOWN;

inline void __atlasLinkNodes( Atlas& a, Key parentIdx, Key childIdx, Child child, uint32_t x, uint32_t y, uint32_t remW, uint32_t remH ) {
  assert( childIdx < a.btP.size() );
  a.btP[childIdx].rect.x = static_cast<int32_t>( x );
  a.btP[childIdx].rect.y = static_cast<int32_t>( y );
  a.btP[childIdx].remW = remW;
  a.btP[childIdx].remH = remH;
  _btLinkNodes( a.btP[parentIdx].header, a.btP[childIdx].header, parentIdx, childIdx, child );
}

// A node's remW is the free width to the right of it on its shelf and remH the free height below it. Linking a
// child gives it that free region and takes it from the parent: a RIGHT_RECT child gets the region right of the
// parent (as tall as the parent plus the room under it), leaving the parent only its own width for anything below;
// a LOWER_RECT child gets the region under the parent, leaving the parent only its own height to its right.
// (jollybean computed the children's room without shrinking the parent's, so a node's right and lower children
// could overlap, and its unsigned remainders could wrap; this keeps the same tree and search.)
void _atlasLinkNodes( Atlas& a, const Key parentIdx, const Key childIdx, Child child ) {
  auto& p = a.btP[parentIdx];
  auto& c = a.btP[childIdx];
  if ( child == RIGHT_RECT ) {
    __atlasLinkNodes( a, parentIdx, childIdx, RIGHT_RECT, p.rect.x + p.rect.w, p.rect.y,
                      p.remW - c.rect.w, p.rect.h + p.remH - c.rect.h );
    p.remW = 0;
  }
  else {  // implicit "if child == LOWER_RECT"
    __atlasLinkNodes( a, parentIdx, childIdx, LOWER_RECT, p.rect.x, p.rect.y + p.rect.h,
                      p.remW + p.rect.w - c.rect.w, p.remH - c.rect.h );
    p.remH = 0;
  }
}

inline auto _rectFitsToTheRight( const Rect& orphanRect, const AtlasElem& parentElem ) -> bool {
  return static_cast<uint32_t>( orphanRect.w ) <= parentElem.remW &&
         static_cast<uint32_t>( orphanRect.h ) <= parentElem.remH + parentElem.rect.h;
}

inline auto _rectFitsBelow( const Rect& orphanRect, const AtlasElem& parentElem ) -> bool {
  return static_cast<uint32_t>( orphanRect.w ) <= parentElem.remW + parentElem.rect.w &&
         static_cast<uint32_t>( orphanRect.h ) <= parentElem.remH;
}

inline void _setRectData( AtlasElem& el, uint32_t maxDim, int32_t w, int32_t h, Key srcIdx ) {
  el.maxDim = maxDim;  // larger of height or width
  el.srcIdx = srcIdx;  // index of sample in original array
  el.rect = Rect{ 0, 0, w, h };
  el.remW = 0;
  el.remH = 0;
}

}  // namespace

// Sort rects by largest dimension. Because the atlas grows in two dimensions as placements are worked out, its
// texture isn't allocated here yet.
auto atlasNew( const std::vector<Size>& sizes, uint32_t pad ) -> Atlas {
  Atlas a;
  const auto N_ATLAS_ELEMS = static_cast<Key>( sizes.size() );
  a.btP.resize( N_ATLAS_ELEMS );  // btNew: every header starts orphaned...
  if ( N_ATLAS_ELEMS == 0 ) return a;
  _btHeaderClr( a.btP[0].header );  // ...except the root's
  auto W = [&]( Key i ) { return static_cast<int32_t>( sizes[i].w + pad ); };
  auto H = [&]( Key i ) { return static_cast<int32_t>( sizes[i].h + pad ); };
  // Populate first element so the next one has something to sort against.
  _setRectData( a.btP[0], std::max( W( 0 ), H( 0 ) ), W( 0 ), H( 0 ), 0 );
  a.extremityA[0] = 0;
  a.extremityA[1] = 0;
  // Loop through the unsorted rectangles
  for ( Key i = 1; i < N_ATLAS_ELEMS; ++i ) {
    const uint32_t currRectMaxDim = std::max( W( i ), H( i ) );
    bool placed = false;
    // Loop through sorted rectangles to see where the current unsorted one should go.
    for ( Key j = 0; j < i; ++j ) {
      if ( currRectMaxDim > a.btP[j].maxDim ) {
        for ( Key k = i; k > j; --k ) a.btP[k] = a.btP[k - 1];  // memmove
        _setRectData( a.btP[j], currRectMaxDim, W( i ), H( i ), i );
        placed = true;
        break;
      }
    }
    // If loop ended without placing rect anywhere, it belongs in last element.
    if ( !placed ) _setRectData( a.btP[i], currRectMaxDim, W( i ), H( i ), i );
  }
  // The memmove can shift the root's header down the array: only element 0 is the root.
  for ( Key i = 0; i < N_ATLAS_ELEMS; ++i ) a.btP[i].header = BtElHeader{};
  _btHeaderClr( a.btP[0].header );
  return a;
}

void atlasPlanPlacements( Atlas& a ) {
  if ( a.btP.empty() ) return;
  // We never search for rect space in orphan nodes, so we must only check the root for orphan-hood.
  assert( !_btIsAnOrphan( a.btP[0].header ) );
  auto& btP = a.btP;
  const Key parentEndIdx = static_cast<Key>( btP.size() );
  Key parentIdx;
  assert( a.extremityA[RIGHT_RECT] == 0 );
  // Initialize the first atlas elem manually so the elem ini magic works branchlessly.
  btP[0].remW = btP[0].rect.w;
  btP[0].remH = btP[0].rect.h;
  // Loop through orphan elements (0th elem gets pre-inserted into atlas)
  for ( Key orphanIdx = 1; orphanIdx < parentEndIdx; ++orphanIdx ) {
    const Rect& orphanRect = btP[orphanIdx].rect;
    parentIdx = a.extremityA[RIGHT_RECT];
  searchForward:  // moves only right and down till a fit or a dead-end is found
    // Since you're going to navigate rightward till you find nothing else to the right, you may as well start
    // your search there.
    while ( parentIdx < parentEndIdx ) {
      const Key* childA = btP[parentIdx].header.childA;
      // Navigate to the right as far as you can go.
      if ( childA[RIGHT_RECT] ) { parentIdx = childA[RIGHT_RECT]; continue; }
      if ( childA[LOWER_RECT] ) { parentIdx = childA[LOWER_RECT]; continue; }
      if ( parentIdx ) {
        if ( _rectFitsToTheRight( orphanRect, btP[parentIdx] ) ) {
          _atlasLinkNodes( a, parentIdx, orphanIdx, RIGHT_RECT );
          goto nextOrphan;
        }
        // Does rect fit beneath? (right in binary tree-speak means rect to the right)
        if ( _rectFitsBelow( orphanRect, btP[parentIdx] ) ) {
          _atlasLinkNodes( a, parentIdx, orphanIdx, LOWER_RECT );
          goto nextOrphan;
        }
      }
      // Dead-end. Back out until an unexplored lower direction is found.
      break;
    }
    // Back out of dead ends: moves only up and left till an unexplored downward direction or root is found.
    while ( parentIdx ) {
      // Avoid re-entering a path you just exited by only going up and left.
      const bool cameFromRight = parentIdx == btP[btP[parentIdx].header.parent].header.childA[RIGHT_RECT];
      // Go to parent node.
      parentIdx = btP[parentIdx].header.parent;
      // If you moved left to get to parent node and see an unexplored path below, go down.
      if ( cameFromRight && !btP[parentIdx].header.childA[LOWER_RECT] && parentIdx && _rectFitsBelow( orphanRect, btP[parentIdx] ) ) {
        _atlasLinkNodes( a, parentIdx, orphanIdx, LOWER_RECT );
        goto nextOrphan;
      }
      if ( cameFromRight && btP[parentIdx].header.childA[LOWER_RECT] && _rectFitsBelow( orphanRect, btP[parentIdx] ) ) {
        parentIdx = btP[parentIdx].header.childA[LOWER_RECT];
        goto searchForward;
      }
    }
    {
      // If no suitable parent was found (parentIdx == 0 now), it's time to expand the atlas.
      const uint32_t todo = ( static_cast<uint32_t>( orphanRect.h ) <= btP[0].remH )                    // can right
                          | ( ( btP[0].remH >= btP[0].remW + orphanRect.w ) << 1 )                      // should right
                          | ( ( static_cast<uint32_t>( orphanRect.w ) <= btP[0].remW ) << 2 )           // can down
                          | ( ( btP[0].remW >= btP[0].remH + orphanRect.h ) << 3 );                     // should down
      switch ( todo ) {
        case SHOULD_RIGHT_CAN_DOWN:
        case SHOULD_RIGHT_DOWN:
        case SHOULD_RIGHT:
        case CAN_RIGHT:
        default: {
          // Grow right: the new image starts a column at the atlas's right edge, from the top.
          const Key ext = a.extremityA[RIGHT_RECT];
          btP[0].remH = std::max<uint32_t>( btP[0].remH, orphanRect.h );
          _btLinkNodes( btP[ext].header, btP[orphanIdx].header, ext, orphanIdx, RIGHT_RECT );
          btP[orphanIdx].rect.x = static_cast<int32_t>( btP[0].remW );
          btP[orphanIdx].rect.y = 0;
          btP[orphanIdx].remW = 0;
          btP[orphanIdx].remH = btP[0].remH - orphanRect.h;
          btP[0].remW += orphanRect.w;
          a.extremityA[RIGHT_RECT] = orphanIdx;
          break;
        }
        case CAN_RIGHT_SHOULD_DOWN:
        case SHOULD_DOWN:
        case CAN_DOWN: {
          // Grow down: the new image starts a shelf at the atlas's bottom edge, from the left.
          const Key ext = a.extremityA[LOWER_RECT];
          btP[0].remW = std::max<uint32_t>( btP[0].remW, orphanRect.w );
          _btLinkNodes( btP[ext].header, btP[orphanIdx].header, ext, orphanIdx, LOWER_RECT );
          btP[orphanIdx].rect.x = 0;
          btP[orphanIdx].rect.y = static_cast<int32_t>( btP[0].remH );
          btP[orphanIdx].remW = btP[0].remW - orphanRect.w;
          btP[orphanIdx].remH = 0;
          btP[0].remH += orphanRect.h;
          a.extremityA[LOWER_RECT] = orphanIdx;  // update the lowest extremity
          break;
        }
      }
    }
  nextOrphan:
    continue;
  }
}

// updateCmSrcRectIndices / _updateSrcRects: each source image's place in the atlas.
auto atlasPlacements( const Atlas& a, uint32_t pad ) -> std::vector<Rect> {
  std::vector<Rect> out( a.btP.size() );
  for ( const auto& e : a.btP ) {
    out[e.srcIdx] = Rect{ e.rect.x, e.rect.y, e.rect.w - static_cast<int32_t>( pad ), e.rect.h - static_cast<int32_t>( pad ) };
  }
  return out;
}

}  // namespace atlas
