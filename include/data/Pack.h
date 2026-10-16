#pragma once
// Packed assets. `cmake --build build` runs the `assets` target, which packs every file under <game>/genome
// (and <game>/main.lua) into <game>/assets.bpak with tools/pack (bicycle-pack), compressing each one:
//   DEFLATE  zlib streams, inflated with jollybean's tinfl (src/data/tinfl.c): Lua genomes, tilemaps, MIDI, sf2
//   STRIP    indexed images (.bimg) in jollybean's strip format: deduplicated strips of pixels, bit-packed, then
//            deflated (src/data/Strip.cpp)
//   STORE    already-compressed data, e.g. meshes (.bmesh, Edgebreaker; src/data/Edgebreaker.cpp)
// The engine reads assets through assets::read(), which looks in the pack first and falls back to loose files
// (set BICYCLE_LOOSE_ASSETS=1 to ignore the pack).
//
// .bpak layout (little endian):
//   "BPAK" u32 version(1) u32 count
//   count x { u16 nameLen, name (relative to the game folder, '/' separated), u8 method, u32 rawLen,
//             u32 dataLen, u32 offset (from file start) }
//   data
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "data/Mem.h"

namespace assets {

enum Method : uint8_t { STORE = 0, DEFLATE = 1, STRIP = 2 };

struct Entry {
  std::string name;
  Method method{};
  uint32_t rawLen{};
  uint32_t dataLen{};
  uint32_t offset{};
};

// Opens <game>/assets.bpak if it exists (and BICYCLE_LOOSE_ASSETS isn't set). Returns whether a pack is mounted.
auto mount( const std::string& gameName ) -> bool;
auto mounted() -> bool;
// Reads an asset by path, e.g. "./test/genome/hero.lua" or "test/genome/music/meadow.sf2": inflated from the
// pack if it's there, else read from disk. nullopt if neither has it.
// The bytes land straight in `pool` (TEXT for Lua, GRAPHIC for images and meshes, AUDIO for sf2/MIDI, ...).
auto read( const std::string& path, mem::PoolId pool = mem::GENERAL ) -> std::optional<mem::Blob>;
auto exists( const std::string& path ) -> bool;
// Pack entries under a folder (e.g. "./test/genome/scene/bg/level1"), as paths like the argument's; empty
// without a pack.
auto list( const std::string& dir ) -> std::vector<std::string>;

// Pack file helpers shared by the engine, tools/pack and the tests.
auto parsePack( std::string_view bytes, std::vector<Entry>& entries ) -> bool;
auto unpackEntry( std::string_view pack, const Entry& e, mem::PoolId pool = mem::GENERAL ) -> std::optional<mem::Blob>;
auto inflateZlib( const uint8_t* src, size_t srcLen, size_t rawLen, mem::PoolId pool = mem::GENERAL ) -> std::optional<mem::Blob>;
// A whole file read straight into a pool.
auto readFile( const std::string& path, mem::PoolId pool ) -> std::optional<mem::Blob>;

}  // namespace assets
