#pragma once
// Pixel images (.bimg) for entities, backgrounds and the HUD: loading, sharing, palette variants, layer
// composites and tilemaps (the image side of newPixelImage and addBgLayerPixels in src/lua/).
#include "data/BImg.h"
#include "v/common/Image.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace pixels {

// A .bimg asset, parsed (null if it's missing or not a .bimg).
auto load( const std::string& path ) -> std::optional<BImg>;
// load(), shared: what the HUD's bar sprites hold on to (null for a null or empty path, or a missing file).
auto loadShared( const char* path ) -> std::shared_ptr<const BImg>;

// An entity's image from .bimg layers (bottom first; Aseprite pixel export): one layer is its graybody, shared by
// every entity drawing it; several are composited in true color. With a palette ("i:rgba,..."), a recolored
// variant, shared by every entity using the same layers and palette. Null if a layer is missing (or, for a
// palette, isn't indexed).
auto sprite( const std::vector<std::string>& layers ) -> std::shared_ptr<const BImg>;
auto sprite( const std::vector<std::string>& layers, const std::string& palette ) -> std::shared_ptr<const BImg>;

// A background from a tileset strip (tile i at x = i * tileW) and tile indices (row-major, nw x nh; 0 = empty).
auto tilemap( const BImg& tileset, const int* tiles, int nw, int nh, int tileW, int tileH ) -> BImg;

// The engine Image for pixels (one pixel is 2x1 cells, see Renderer.h); preloads them into the renderer's atlas.
auto asImage( std::shared_ptr<const BImg> px ) -> Image;

// Newline-separated paths, empty lines skipped.
auto splitLines( const char* paths ) -> std::vector<std::string>;

}  // namespace pixels
