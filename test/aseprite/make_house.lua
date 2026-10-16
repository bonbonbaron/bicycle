-- Authors test/aseprite/house.aseprite: the cottage interior (the second scene, test/genome/scene/house.lua), a
-- background with tilemap layers like meadow.aseprite: a back wall with windows and a shelf, a plank floor, and an
-- open doorway on the left (the way back to the meadow).
-- Each layer's mode is set in its user data, per scripts/aseprite/README.txt.
-- Run in batch mode:  aseprite -b --script test/aseprite/make_house.lua
local here = debug.getinfo(1, "S").source:match("^@?(.*[/\\])") or "./"

-- Tiles are 4x2 px; the exporter turns each pixel into 2 ASCII chars, so a tile is 8x2 chars.
local TILE_W, TILE_H = 4, 2
local TILES_WIDE, TILES_HIGH = 24, 25   -- 192x50 chars

-- As in make_meadow.lua: a gray whose luminance maps to the wanted ASCII char in the exporter's ramp.
local RAMP = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. "
local function colorFor( ch )
  if ch == " " then return Color{ r = 255, g = 255, b = 255, a = 0 } end
  local idx = RAMP:find( ch, 1, true )
  assert( idx, "char not in ramp: " .. ch )
  local g = math.floor( ( idx - 1 ) / ( #RAMP - 1 ) * 255 + 0.5 )
  return Color{ r = g, g = g, b = g, a = 255 }
end

local spr = Sprite( TILES_WIDE * TILE_W, TILES_HIGH * TILE_H, ColorMode.RGB )
spr.gridBounds = Rectangle( 0, 0, TILE_W, TILE_H )
local defaultLayer = spr.layers[1]

local function addLayer( name, userData, tiles, place )
  app.command.NewLayer{ name = name, tilemap = true }
  local layer = app.layer
  layer.data = userData
  for _, rows in ipairs( tiles ) do
    local img = Image( TILE_W, TILE_H, ColorMode.RGB )
    for y, row in ipairs( rows ) do
      for x = 1, TILE_W do img:drawPixel( x - 1, y - 1, colorFor( row:sub( x, x ) ) ) end
    end
    local tile = spr:newTile( layer.tileset )
    tile.image = img
  end
  local map = Image( TILES_WIDE, TILES_HIGH, ColorMode.TILEMAP )
  for row = 0, TILES_HIGH - 1 do
    for col = 0, TILES_WIDE - 1 do
      local t = place( col, row )
      if t then map:putPixel( col, row, app.pixelColor.tile( t, 0 ) ) end
    end
  end
  spr:newCel( layer, 1, map, Point( 0, 0 ) )
  return layer
end

local DOORWAY = 1  -- tile column of the open doorway (left wall)
-- Bottom to top.
-- Back wall: brick courses, two windows and a shelf; the doorway is left open.
addLayer( "wall", "mode=fixed",
  { { "+#++", "++#+" },            -- 1 bricks
    { "[--]", "|  |" },            -- 2 window top
    { "|  |", "[--]" },            -- 3 window bottom
    { "____", "    " },            -- 4 shelf
    { " oi ", "____" } },          -- 5 shelf with a jar and a candle
  function( c, r )
    if r < 3 or r > 22 then return nil end
    if c == DOORWAY and r >= 15 then return nil end  -- the doorway
    if ( c == 6 or c == 7 or c == 15 or c == 16 ) and r == 8 then return 2 end
    if ( c == 6 or c == 7 or c == 15 or c == 16 ) and r == 9 then return 3 end
    if c >= 10 and c <= 12 and r == 13 then return 5 end
    if r % 3 == 0 then return 1 end
  end )
-- Floor planks and the skirting.
addLayer( "floor", "mode=fixed", { { "MMMM", "wwww" }, { "----", "    " } }, function( c, r )
  if r >= 23 then return 1 end
  if r == 22 and c ~= DOORWAY then return 2 end
end )

spr:deleteLayer( defaultLayer )
spr:saveAs( here .. "house.aseprite" )
print( "wrote " .. here .. "house.aseprite" )
