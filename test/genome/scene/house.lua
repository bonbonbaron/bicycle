-- The second scene: inside the cottage (art: test/aseprite/make_house.lua). The doorway on the left leads back out.
local house = {}
house.name = "Cottage"
house.bg = "house"
house.scenery3d = "house3d"   -- 3D: test/genome/level/house3d.lua (floor + walls) instead of the meadow
house.spawns = { entry = { 20, 40 } }
house.fg = {
  [2] = {
    { genome = "hero", pos = { 20, 40 }, focus = true },  -- only used if you start here; else the hero walks in
    { genome = "fairy", pos = { 10, 35 } },
    { genome = "keepsake", pos = { 120, 42 }, key = "keepsake" },
    { genome = "door", pos = { 2, 36 }, door = { to = "intro", spawn = "from_house", w = 8, h = 12 } },
  }
}
house.music = { soundfont = "music/meadow.sf2", song = "music/house.mid" }
house.sfx = { soundfont = "music/meadow.sf2", program = 120, sounds = { bump = 60, coin = 72, hurt = 55, blip = 76 } }
return house
