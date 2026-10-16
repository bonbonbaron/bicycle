-- Authors test/aseprite/beetle_sprite.aseprite: a 6x4 px beetle with a 2-frame "crawl" tag (the linear patrol demo).
-- Run in batch mode:  aseprite -b --script test/aseprite/make_beetle.lua
-- Without Aseprite: lua5.4 tools/aseprite/headless.lua test/aseprite/make_beetle.lua
local here = debug.getinfo(1, "S").source:match("^@?(.*[/\\])") or "./"
local PALETTE = {
  ["."] = Color{ r = 255, g = 255, b = 255, a = 0 },
  G = Color{ r = 40,  g = 140, b = 70,  a = 255 },   -- shell
  g = Color{ r = 120, g = 210, b = 120, a = 255 },   -- shine
  K = Color{ r = 25,  g = 25,  b = 25,  a = 255 },   -- head, legs
}
local FRAMES = {
  { ".GgG..", "GGGGGK", "GGGGGK", "K.K.K." },
  { ".GgG..", "GGGGGK", "GGGGGK", ".K.K.K" },
}
local spr = Sprite( 6, 4, ColorMode.RGB )
local layer = spr.layers[1]
layer.name = "body"
for _ = 2, #FRAMES do spr:newEmptyFrame() end
for f, rows in ipairs( FRAMES ) do
  local img = Image( 6, 4, ColorMode.RGB )
  for y, row in ipairs( rows ) do
    for x = 1, 6 do img:drawPixel( x - 1, y - 1, PALETTE[row:sub( x, x )] ) end
  end
  spr.frames[f].duration = 0.2
  spr:newCel( layer, spr.frames[f], img, Point( 0, 0 ) )
end
spr:newTag( 1, #FRAMES ).name = "crawl"
spr:saveAs( here .. "beetle_sprite.aseprite" )
print( "wrote " .. here .. "beetle_sprite.aseprite" )
