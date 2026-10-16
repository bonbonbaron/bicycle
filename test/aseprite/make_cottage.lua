-- Authors test/aseprite/cottage_sprite.aseprite: a 14x12 px cottage (one frame, tag "idle") that marks the meadow's
-- door into the house scene (the door itself is an invisible trigger: test/genome/door.lua).
-- Run in batch mode:  aseprite -b --script test/aseprite/make_cottage.lua
local here = debug.getinfo(1, "S").source:match("^@?(.*[/\\])") or "./"
local PALETTE = {
  ["."] = Color{ r = 255, g = 255, b = 255, a = 0 },
  R = Color{ r = 170, g = 60,  b = 45,  a = 255 },   -- roof
  r = Color{ r = 120, g = 40,  b = 30,  a = 255 },   -- roof shade
  W = Color{ r = 225, g = 205, b = 160, a = 255 },   -- walls
  w = Color{ r = 185, g = 165, b = 125, a = 255 },   -- wall shade
  D = Color{ r = 95,  g = 60,  b = 35,  a = 255 },   -- door
  K = Color{ r = 240, g = 200, b = 80,  a = 255 },   -- knob, lit window
  B = Color{ r = 60,  g = 90,  b = 140, a = 255 },   -- window frame
  C = Color{ r = 110, g = 110, b = 110, a = 255 },   -- chimney
}
local ROWS = {
  "..........CC..",
  "......RR..CC..",
  ".....RRRR.CC..",
  "....RRRRRRRR..",
  "...RRRRRRRRRR.",
  "..rRRRRRRRRRRr",
  ".rrrrrrrrrrrrr",
  ".WWWWWWWWWWWW.",
  ".WBKBWWDDWBKBw",
  ".WBBBWWDDWBBBw",
  ".WWWWWWDKWWWWw",
  ".wwwwwwDDwwwww",
}
local spr = Sprite( 14, 12, ColorMode.RGB )
local layer = spr.layers[1]
layer.name = "body"
local img = Image( 14, 12, ColorMode.RGB )
for y, row in ipairs( ROWS ) do
  for x = 1, 14 do img:drawPixel( x - 1, y - 1, PALETTE[row:sub( x, x )] ) end
end
spr.frames[1].duration = 1.0
spr:newCel( layer, spr.frames[1], img, Point( 0, 0 ) )
spr:newTag( 1, 1 ).name = "idle"
spr:saveAs( here .. "cottage_sprite.aseprite" )
print( "wrote " .. here .. "cottage_sprite.aseprite" )
