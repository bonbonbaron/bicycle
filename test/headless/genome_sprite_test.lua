-- Headless check that Genome.lua reads scripts/aseprite outputs. Run from the repo root:
--   luajit test/headless/genome_sprite_test.lua
package.path = package.path .. ";./scripts/?.lua"
local Genome = require("Genome")
local Sprite = Genome.Sprite
gameName = "test"

local function check( cond, msg )
  if not cond then error( "FAIL: "..msg, 2 ) end
end

local function sortedKeys( t )
  local keys = {}
  for k, _ in pairs( t ) do table.insert( keys, k ) end
  table.sort( keys )
  return keys
end

local function describe( spr )
  print( "sprite '"..spr.name.."' is a "..spr.kind.." with "..#spr.layers.." layer(s)" )
  for i, layer in ipairs( spr.layers ) do
    if layer.kind == Sprite.CHARACTER then
      print( string.format( "  layer %d: %d frame(s), each %dx%d chars", i, #layer.frames, layer.w, layer.h ) )
      for _, name in ipairs( sortedKeys( layer.strips ) ) do
        local strip = layer.strips[name]
        local parts = {}
        for _, f in ipairs( strip.frames ) do table.insert( parts, f.frameIdx.."@"..f.durationMs.."ms" ) end
        print( "    strip "..name..": "..table.concat( parts, " " ).."  (play order "..table.concat( strip.sequence, "," )..")" )
      end
    else
      print( string.format( "  layer %d: %dx%d tiles of %dx%d px", i, layer.numTilesWide, layer.numTilesHigh, layer.tileW, layer.tileH ) )
      print( Sprite.tilemapToAscii( layer ) )
    end
  end
end

-- 1. The real export committed by sprite.lua (untagged, 4 frames).
local spr = Sprite.load( "Sprite_0001" )
describe( spr )
local layer = spr.layers[1]
check( spr.kind == Sprite.CHARACTER, "Sprite_0001 should be a character" )
check( #layer.frames == 4, "Sprite_0001 should have 4 frames" )
check( layer.w == 64 and layer.h == 32, "Sprite_0001 frames should be 64x32 chars (32x32 px, 2 chars per px)" )
check( layer.strips.DEFAULT and #layer.strips.DEFAULT.frames == 4, "Sprite_0001 should have a 4-frame DEFAULT strip" )
check( layer.strips.DEFAULT.frames[1].durationMs == 100, "frame duration should convert 0.1s to 100ms" )
for _, frame in ipairs( layer.frames ) do
  for line in ( frame.."\n" ):gmatch( "(.-)\n" ) do check( #line == 64, "every frame row should be 64 chars" ) end
end
print( "  first frame, first 4 rows:" )
local n = 0
for line in ( layer.frames[1].."\n" ):gmatch( "(.-)\n" ) do
  n = n + 1
  if n > 4 then break end
  print( "    |"..line.."|" )
end

-- 2. Tagged, multi-direction character (hand-made fixture in the same format).
local walker = Sprite.load( "walker" )
describe( walker )
check( walker.kind == Sprite.CHARACTER, "walker should be a character" )
for _, dir in ipairs( { "walk_down", "walk_up", "walk_left", "walk_right" } ) do
  check( walker.layers[1].strips[dir], "walker should have strip "..dir )
end
check( walker.layers[1].strips.walk_left.frames[1].frameIdx == 5, "walk_left should start at frame 5" )
check( walker.layers[1].frames[5] == " (o ) \n-|##| \n /  \\ ", "walker frame 5 should be sliced exactly" )

-- 3. Tilemap through the same interface.
local room = Sprite.load( "room" )
describe( room )
check( room.kind == Sprite.TILEMAP, "room should be a tilemap" )
check( Sprite.tilemapToAscii( room.layers[1] ) == "....\n. #.\n....", "room should render to glyphs" )
check( room.layers[1].collmap[1] == "WALL", "room collmap should load" )

-- 4. Image layers composite in order; spaces are transparent.
local strips = { DEFAULT = { frames = { { frameIdx = 1, durationMs = 100 } }, sequence = { 1 } } }
local bottom = { kind = Sprite.CHARACTER, frames = { "aaaa\naaaa" }, strips = strips, w = 4, h = 2 }
local top    = { kind = Sprite.CHARACTER, frames = { " b  \n  bb" }, strips = strips, w = 4, h = 2 }
check( Sprite.composite( { bottom, top } ).frames[1] == "abaa\naabb", "layers should composite with spaces transparent" )

-- 5. Tilemaps draw from the tileset's ASCII tiles when exported (tile 0 = empty).
local tm = { numTilesWide = 2, numTilesHigh = 1, tileW = 1, tileH = 2, tiles = { 1, 0 },
             tileset = { ascii = { "  \n  ", "##\n##" } } }
check( Sprite.tilemapToAscii( tm ) == "##  \n##  ", "tilemap should render from ASCII tiles" )

print( "ALL GENOME SPRITE TESTS PASSED" )
