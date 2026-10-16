-- Authors test/aseprite/fairy_sprite.aseprite: a 7x7 px sparkly fairy with a 4-frame "flutter" tag (wings beat up and
-- down, a sparkle twinkles around her). Run in batch mode:
--   aseprite -b --script test/aseprite/make_fairy.lua
--   aseprite -b test/aseprite/fairy_sprite.aseprite --script scripts/aseprite/sprite.lua
-- Without Aseprite: lua5.4 tools/aseprite/headless.lua test/aseprite/make_fairy.lua (writes the same genome files).
local here = debug.getinfo(1, "S").source:match("^@?(.*[/\\])") or "./"

local PALETTE = {
  ["."] = Color{ r = 255, g = 255, b = 255, a = 0 },  -- transparent
  W = Color{ r = 120, g = 200, b = 255, a = 255 },   -- wings
  w = Color{ r = 200, g = 240, b = 255, a = 255 },   -- wing highlight
  H = Color{ r = 240, g = 170, b = 40,  a = 255 },   -- hair
  S = Color{ r = 250, g = 205, b = 175, a = 255 },   -- skin
  D = Color{ r = 210, g = 60,  b = 160, a = 255 },   -- dress
  K = Color{ r = 40,  g = 20,  b = 60,  a = 255 },   -- eyes / outline
  Y = Color{ r = 255, g = 250, b = 120, a = 255 },   -- sparkle
}

local FRAMES = {
  { "Ww...wW", "WW.H.WW", ".WWSWW.", "...D...", "..DDD..", "...K...", "Y......" },  -- wings up
  { ".......", "Ww.H.wW", "WWWSWWW", "...D...", "..DDD..", "...K..Y", "......." },  -- wings level
  { ".......", "...H...", ".WWSWW.", "WW.D.WW", "Ww.DDwW", "...K...", "...Y..." },  -- wings down
  { "......Y", "Ww.H.wW", "WWWSWWW", "...D...", "..DDD..", "...K...", "......." },  -- wings level
}

local spr = Sprite( 7, 7, ColorMode.RGB )
local layer = spr.layers[1]
layer.name = "body"
for _ = 2, #FRAMES do spr:newEmptyFrame() end
for f, rows in ipairs( FRAMES ) do
  local img = Image( 7, 7, ColorMode.RGB )
  for y, row in ipairs( rows ) do
    for x = 1, 7 do img:drawPixel( x - 1, y - 1, PALETTE[row:sub( x, x )] ) end
  end
  spr.frames[f].duration = 0.09
  spr:newCel( layer, spr.frames[f], img, Point( 0, 0 ) )
end
spr:newTag( 1, #FRAMES ).name = "flutter"
spr:saveAs( here .. "fairy_sprite.aseprite" )
print( "wrote " .. here .. "fairy_sprite.aseprite" )
