-- Authors test/aseprite/meadow.aseprite: a background with one tilemap layer per background mode.
-- Each layer's mode is set in its user data (Layer Properties > user data), per scripts/aseprite/README.txt.
-- Run in batch mode:  aseprite -b --script test/aseprite/make_meadow.lua
local here = debug.getinfo(1, "S").source:match("^@?(.*[/\\])") or "./"

-- Tiles are 4x2 px; the exporter turns each pixel into 2 ASCII chars, so a tile is 8x2 chars.
local TILE_W, TILE_H = 4, 2
local TILES_WIDE, TILES_HIGH = 40, 25   -- 320x50 chars of world

-- To get a particular ASCII char, paint the gray whose luminance maps to it in the exporter's "detailed" ramp.
-- " " is left transparent.
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
local defaultLayer = spr.layers[1]  -- new sprites start with an image layer; backgrounds use only tilemaps

-- Adds a tilemap layer. tiles: list of 2-row pixel patterns (tile 1, 2, ...). place(col, row) returns a tile
-- number (or nil) for each map cell.
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

-- Bottom to top (Aseprite draws the first layer at the back).
addLayer( "sky", "mode=glued", { { "`   ", "    " }, { "    ", "  . " } }, function( c, r )
  if r < 12 and ( c * 7 + r * 3 ) % 11 == 0 then return 1 + ( c + r ) % 2 end
end )
addLayer( "clouds", "mode=autoloop vx=6", { { " ()(", "(___" }, { ")() ", "___)" } }, function( c, r )
  if ( r == 2 and ( c == 3 or c == 21 ) ) or ( r == 5 and c == 12 ) then return 1 end
  if ( r == 2 and ( c == 4 or c == 22 ) ) or ( r == 5 and c == 13 ) then return 2 end
end )
addLayer( "hills", "mode=parallax parallax=50", { { "  /\\", " /  " }, { "    ", "\\   " } }, function( c, r )
  if r == 10 and c % 3 == 0 then return 1 end
  if r == 10 and c % 3 == 1 then return 2 end
end )
addLayer( "ground", "mode=fixed", { { "wwww", "MMMM" }, { " @@ ", " || " }, { "    ", " oO " } }, function( c, r )
  if r >= 23 then return 1 end
  if r == 21 and c % 7 == 2 then return 2 end
  if r == 22 and c % 7 == 2 then return 2 end
  if r == 22 and c % 5 == 4 then return 3 end
end )
addLayer( "fence", "mode=loop parallax=150 parallaxy=100", { { "|-|-", "| | " } }, function( c, r )
  if r == 24 and c < 10 then return 1 end
end )

spr:deleteLayer( defaultLayer )
spr:saveAs( here .. "meadow.aseprite" )
print( "wrote " .. here .. "meadow.aseprite" )
