-- Authors test/aseprite/pilgrim.aseprite: an 8x8 character with two layers (body, head) and
-- walk_down/walk_up/walk_left/walk_right tags, 2 frames each. Run in batch mode:
--   aseprite -b --script test/aseprite/make_pilgrim.lua
local here = debug.getinfo(1, "S").source:match("^@?(.*[/\\])") or "./"

local PALETTE = {
  ["."] = Color{ r = 255, g = 255, b = 255, a = 0 },  -- transparent
  K = Color{ r = 20,  g = 20,  b = 20,  a = 255 },   -- outline / hair
  G = Color{ r = 110, g = 110, b = 140, a = 255 },   -- tunic
  L = Color{ r = 190, g = 190, b = 200, a = 255 },   -- hands
  S = Color{ r = 230, g = 185, b = 150, a = 255 },   -- skin
}

local HEAD = {
  down  = { "..KKKK..", ".KSKKSK.", "..KSSK.." },
  up    = { "..KKKK..", ".KKKKKK.", "..KKKK.." },
  left  = { "..KKKK..", ".KSKKKK.", "..KSKK.." },
  right = { "..KKKK..", ".KKKKSK.", "..KKSK.." },
}
local BODY = {
  down  = { { "..GGGG..", ".LGGGGL.", "..GGGG..", "..G..G..", ".KK...K." },
            { "..GGGG..", ".LGGGGL.", "..GGGG..", "..G..G..", ".K...KK." } },
  up    = { { "..GGGG..", ".LGGGGL.", "..GGGG..", "..G..G..", ".K...KK." },
            { "..GGGG..", ".LGGGGL.", "..GGGG..", "..G..G..", ".KK...K." } },
  left  = { { "..GGGG..", ".LGGGG..", "..GGGG..", "..G..G..", ".KK..K.." },
            { "..GGGG..", ".LGGGG..", "..GGGG..", "...GG...", "..KK...." } },
  right = { { "..GGGG..", "..GGGGL.", "..GGGG..", "..G..G..", "..K..KK." },
            { "..GGGG..", "..GGGGL.", "..GGGG..", "...GG...", "....KK.." } },
}
local DIRS = { "down", "up", "left", "right" }

local function paint( rows, yOffset )
  local img = Image( 8, 8, ColorMode.RGB )
  for y, row in ipairs( rows ) do
    for x = 1, #row do
      img:drawPixel( x - 1, yOffset + y - 1, PALETTE[ row:sub( x, x ) ] )
    end
  end
  return img
end

local spr = Sprite( 8, 8, ColorMode.RGB )
local body = spr.layers[1]
body.name = "pilgrim body"
local head = spr:newLayer()
head.name = "pilgrim head"
for _ = 2, 2 * #DIRS do spr:newEmptyFrame() end

for d, dir in ipairs( DIRS ) do
  local first = 2 * d - 1
  for step = 1, 2 do
    local frame = spr.frames[ first + step - 1 ]
    frame.duration = 0.2
    spr:newCel( body, frame, paint( BODY[dir][step], 3 ), Point( 0, 0 ) )
    spr:newCel( head, frame, paint( HEAD[dir], 0 ), Point( 0, 0 ) )
  end
  local tag = spr:newTag( first, first + 1 )
  tag.name = "walk_" .. dir
end

spr:saveAs( here .. "pilgrim.aseprite" )
print( "wrote " .. here .. "pilgrim.aseprite" )
