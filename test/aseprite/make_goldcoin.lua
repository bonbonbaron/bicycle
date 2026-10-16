-- Authors test/aseprite/goldcoin.aseprite: a 3x3 px coin with a 4-frame "spin" tag.
-- Run in batch mode:  aseprite -b --script test/aseprite/make_coin.lua
local here = debug.getinfo(1, "S").source:match("^@?(.*[/\\])") or "./"
-- Paint the gray whose luminance maps to each char in the exporter's "detailed" ramp; " " = transparent.
local RAMP = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. "
local function colorFor( ch )
  if ch == " " then return Color{ r = 255, g = 255, b = 255, a = 0 } end
  local g = math.floor( ( RAMP:find( ch, 1, true ) - 1 ) / ( #RAMP - 1 ) * 255 + 0.5 )
  return Color{ r = g, g = g, b = g, a = 255 }
end
local FRAMES = {
  { " O ", "O$O", " O " },
  { " O ", " $ ", " O " },
  { " | ", " | ", " | " },
  { " O ", " $ ", " O " },
}
local spr = Sprite( 3, 3, ColorMode.RGB )
local layer = spr.layers[1]
layer.name = "coin"
for _ = 2, #FRAMES do spr:newEmptyFrame() end
for f, rows in ipairs( FRAMES ) do
  local img = Image( 3, 3, ColorMode.RGB )
  for y, row in ipairs( rows ) do
    for x = 1, 3 do img:drawPixel( x - 1, y - 1, colorFor( row:sub( x, x ) ) ) end
  end
  spr.frames[f].duration = 0.15
  spr:newCel( layer, spr.frames[f], img, Point( 0, 0 ) )
end
spr:newTag( 1, #FRAMES ).name = "spin"
spr:saveAs( here .. "goldcoin.aseprite" )
print( "wrote " .. here .. "goldcoin.aseprite" )
