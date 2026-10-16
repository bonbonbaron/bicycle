-- Authors test/aseprite/elder.aseprite: an 8x8 old villager leaning on a staff, with a 2-frame "idle" tag
-- (he strokes his beard). Run in batch mode:
--   aseprite -b --script test/aseprite/make_elder.lua
local here = debug.getinfo(1, "S").source:match("^@?(.*[/\\])") or "./"

local PALETTE = {
  ["."] = Color{ r = 255, g = 255, b = 255, a = 0 },  -- transparent
  K = Color{ r = 20,  g = 20,  b = 20,  a = 255 },   -- outline / boots
  W = Color{ r = 235, g = 235, b = 235, a = 255 },   -- white hair and beard
  S = Color{ r = 230, g = 185, b = 150, a = 255 },   -- skin
  R = Color{ r = 120, g = 60,  b = 40,  a = 255 },   -- robe
  T = Color{ r = 160, g = 120, b = 60,  a = 255 },   -- staff
}

local FRAMES = {
  { ".WWWW..T", "WSKSKW.T", ".WWWWS.T", "..WWRRRT", ".RRRRR.T", ".RRRRR.T", ".RR.RR.T", ".KK.KK.T" },
  { ".WWWW..T", "WSKSKW.T", ".WWWW..T", "..WWRRST", ".RRRRR.T", ".RRRRR.T", ".RR.RR.T", ".KK.KK.T" },
}

local spr = Sprite( 8, 8, ColorMode.RGB )
local layer = spr.layers[1]
layer.name = "body"
for _ = 2, #FRAMES do spr:newEmptyFrame() end
for f, rows in ipairs( FRAMES ) do
  local img = Image( 8, 8, ColorMode.RGB )
  for y, row in ipairs( rows ) do
    for x = 1, 8 do img:drawPixel( x - 1, y - 1, PALETTE[row:sub( x, x )] ) end
  end
  spr.frames[f].duration = 0.6
  spr:newCel( layer, spr.frames[f], img, Point( 0, 0 ) )
end
spr:newTag( 1, #FRAMES ).name = "idle"
spr:saveAs( here .. "elder.aseprite" )
print( "wrote " .. here .. "elder.aseprite" )
