-- Authors test/aseprite/butterfly_sprite.aseprite: a 5x4 px butterfly with a 2-frame "flap" tag (the motion demos'
-- orbiting and orbit-while-travelling critters). Run in batch mode:
--   aseprite -b --script test/aseprite/make_butterfly.lua
-- Without Aseprite: lua5.4 tools/aseprite/headless.lua test/aseprite/make_butterfly.lua
local here = debug.getinfo(1, "S").source:match("^@?(.*[/\\])") or "./"
local PALETTE = {
  ["."] = Color{ r = 255, g = 255, b = 255, a = 0 },
  O = Color{ r = 245, g = 140, b = 30,  a = 255 },   -- wings
  o = Color{ r = 255, g = 210, b = 90,  a = 255 },   -- wing spots
  K = Color{ r = 30,  g = 20,  b = 20,  a = 255 },   -- body
}
local FRAMES = {
  { "OO.OO", "OoKoO", ".OKO.", "..K.." },  -- open
  { ".....", ".OKO.", ".oKo.", "..K.." },  -- closed
}
local spr = Sprite( 5, 4, ColorMode.RGB )
local layer = spr.layers[1]
layer.name = "body"
for _ = 2, #FRAMES do spr:newEmptyFrame() end
for f, rows in ipairs( FRAMES ) do
  local img = Image( 5, 4, ColorMode.RGB )
  for y, row in ipairs( rows ) do
    for x = 1, 5 do img:drawPixel( x - 1, y - 1, PALETTE[row:sub( x, x )] ) end
  end
  spr.frames[f].duration = 0.12
  spr:newCel( layer, spr.frames[f], img, Point( 0, 0 ) )
end
spr:newTag( 1, #FRAMES ).name = "flap"
spr:saveAs( here .. "butterfly_sprite.aseprite" )
print( "wrote " .. here .. "butterfly_sprite.aseprite" )
