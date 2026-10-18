-- The second scene: inside the cottage (art: test/aseprite/make_house.lua). The doorway on the left leads back out.
local house = {}
house.name = "Cottage"
house.bg = "house"
house.scenery3d = "house3d"   -- 3D: test/genome/level/house3d.lua (floor + walls) instead of the meadow
house.spawns = { entry = { 20, 40 } }
house.fg = {
  [2] = {
    -- BEGIN entities (generated from the entities layer of house.aseprite: place them there)
    { genome = "hero", pos = { 20, 40 }, focus = true },
    { genome = "fairy", pos = { 10, 35 } },
    { genome = "keepsake", pos = { 120, 42 }, key = "keepsake" },
    { genome = "door", pos = { 2, 36 }, door = { h = 12, spawn = "from_house", to = "intro", w = 8 } },
    -- END entities
  }
}
house.music = { soundfont = "meadow", song = "house" }
return house
