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

-- 3b. Real Aseprite export (test/aseprite/pilgrim.aseprite): two layers composited, four walk tags.
local pilgrim = Sprite.load( "pilgrim" )
describe( pilgrim )
check( pilgrim.kind == Sprite.CHARACTER and #pilgrim.layers == 2, "pilgrim should be a 2-layer character" )
check( #pilgrim.character.frames == 8 and pilgrim.character.w == 16 and pilgrim.character.h == 8, "pilgrim should have 8 frames of 16x8 chars" )
for _, dir in ipairs( { "walk_down", "walk_up", "walk_left", "walk_right" } ) do
  check( pilgrim.character.strips[dir], "pilgrim should have strip "..dir )
end
print( pilgrim.character.frames[1] )

-- 3c. What C++'s Animator gets: one image with frames side by side, and each frame's src rect in it.
local stripImg, rects = Sprite.toStrip( pilgrim.character )
check( #rects == 8 and rects[2].x == 17 and rects[2].w == 16 and rects[2].h == 8, "pilgrim src rects should step by frame width + separator" )
local firstRow = stripImg:match( "^[^\n]*" )
check( #firstRow == 8 * 16 + 7, "pilgrim strip image should be 8 frames + 7 separators wide" )
for f, rect in ipairs( rects ) do
  local row1 = pilgrim.character.frames[f]:match( "^[^\n]*" )
  check( firstRow:sub( rect.x + 1, rect.x + rect.w ) == row1, "src rect "..f.." should cover frame "..f )
end

-- 3d. Real Aseprite background (test/aseprite/meadow.aseprite): one tilemap layer per background mode.
local meadow = Sprite.load( "meadow" )
check( meadow.kind == Sprite.TILEMAP and #meadow.layers == 5, "meadow should be a 5-layer tilemap" )
local expectModes = { "glued", "autoloop", "parallax", "fixed", "loop" }
for i, tm in ipairs( meadow.layers ) do
  check( tm.bg.mode == expectModes[i], "meadow layer "..i.." should be "..expectModes[i]..", got "..tm.bg.mode )
  local str = Sprite.tilemapToAscii( tm )
  local nRows, width = 0, 0
  for line in ( str.."\n" ):gmatch( "(.-)\n" ) do nRows = nRows + 1; width = math.max( width, #line ) end
  check( nRows == 50 and width == 320, "meadow layer "..i.." should render 320x50 chars, got "..width.."x"..nRows )
  print( string.format( "  meadow layer %d: %s parallax=%d/%d v=(%g,%g)", i, tm.bg.mode, tm.bg.parallax, tm.bg.parallaxy, tm.bg.vx, tm.bg.vy ) )
end
check( meadow.layers[3].bg.parallax == 50 and meadow.layers[2].bg.vx == 6, "meadow settings should come from layer user data" )
check( meadow.layers[5].bg.parallax == 150 and meadow.layers[5].bg.parallaxy == 100, "fence should loop at 150%/100%" )
check( Sprite.tilemapToAscii( meadow.layers[4] ):find( "wwwwwwww", 1, true ), "ground should draw grass tiles" )
check( room.layers[1].bg.mode == "fixed", "tilemaps without settings default to fixed" )

-- 4. Image layers composite in order; spaces are transparent.
local strips = { DEFAULT = { frames = { { frameIdx = 1, durationMs = 100 } }, sequence = { 1 } } }
local bottom = { kind = Sprite.CHARACTER, frames = { "aaaa\naaaa" }, strips = strips, w = 4, h = 2 }
local top    = { kind = Sprite.CHARACTER, frames = { " b  \n  bb" }, strips = strips, w = 4, h = 2 }
check( Sprite.composite( { bottom, top } ).frames[1] == "abaa\naabb", "layers should composite with spaces transparent" )

-- 5. Tilemaps draw from the tileset's ASCII tiles when exported (tile 0 = empty).
local tm = { numTilesWide = 2, numTilesHigh = 1, tileW = 1, tileH = 2, tiles = { 1, 0 },
             tileset = { ascii = { "  \n  ", "##\n##" } } }
check( Sprite.tilemapToAscii( tm ) == "##  \n##  ", "tilemap should render from ASCII tiles" )

-- 6. Demo coin (test/aseprite/goldcoin.aseprite): a tiny one-tag animated character, separate from the coin genome.
local goldcoin = Sprite.load( "goldcoin" )
describe( goldcoin )
check( goldcoin.kind == Sprite.CHARACTER and #goldcoin.character.frames == 4, "goldcoin should be a 4-frame character" )
check( goldcoin.character.strips.spin and #goldcoin.character.strips.spin.frames == 4, "goldcoin should have a 4-frame spin strip" )
check( Genome.COLL and Genome.COLL.hero and Genome.COLL.solid and Genome.COLL.coin, "Genome.COLL should name collider types" )

-- 7. NPC sprite (test/aseprite/elder.aseprite): one layer, a 2-frame idle.
local elder = Sprite.load( "elder" )
describe( elder )
check( elder.kind == Sprite.CHARACTER and elder.character.strips.idle and #elder.character.strips.idle.frames == 2, "elder should idle in 2 frames" )
check( Genome.COLL.enemy and Genome.COLL.npc, "Genome.COLL should name enemy and npc colliders" )

print( "ALL GENOME SPRITE TESTS PASSED" )
