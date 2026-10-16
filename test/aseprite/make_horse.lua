-- Authors test/aseprite/horse_sprite.aseprite: a 12x6 brown horse (named horse_sprite so its exports don't overwrite the horse genome, test/genome/horse.lua) facing right with a 2-frame "walk" tag. It's the pixel
-- counterpart of test/genome/body/graybody/colormap/horse.txt (the hand-drawn ASCII horse the ASCII mode keeps using; see the
-- horse genome's sprite2d gene). Run in batch mode:
--   aseprite -b --script test/aseprite/make_horse.lua
local here = debug.getinfo(1, "S").source:match("^@?(.*[/\\])") or "./"

local PALETTE = {
  ["."] = Color{ r = 255, g = 255, b = 255, a = 0 },  -- transparent
  B = Color{ r = 150, g = 95,  b = 50,  a = 255 },   -- coat
  M = Color{ r = 70,  g = 40,  b = 20,  a = 255 },   -- mane and tail
  E = Color{ r = 15,  g = 10,  b = 10,  a = 255 },   -- eye
  K = Color{ r = 40,  g = 30,  b = 25,  a = 255 },   -- hooves
}

local FRAMES = {
  { ".........MBB", "M.......MBEB", "MBBBBBBBBB..", ".BBBBBBBB...", ".B.B..B.B...", ".K.K..K.K..." },
  { ".........MBB", ".M......MBEB", "MBBBBBBBBB..", ".BBBBBBBB...", "B..B.B..B...", "K..K.K..K..." },
}
local W, H = 12, 6

local spr = Sprite( W, H, ColorMode.RGB )
local layer = spr.layers[1]
layer.name = "body"
for _ = 2, #FRAMES do spr:newEmptyFrame() end
for f, rows in ipairs( FRAMES ) do
  local img = Image( W, H, ColorMode.RGB )
  for y, row in ipairs( rows ) do
    for x = 1, W do img:drawPixel( x - 1, y - 1, PALETTE[row:sub( x, x )] ) end
  end
  spr.frames[f].duration = 0.25
  spr:newCel( layer, spr.frames[f], img, Point( 0, 0 ) )
end
spr:newTag( 1, #FRAMES ).name = "walk"
spr:saveAs( here .. "horse_sprite.aseprite" )
print( "wrote " .. here .. "horse_sprite.aseprite" )
