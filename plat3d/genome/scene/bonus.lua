-- plat3d's second scene: a walled bonus room with blocks and a ring of coins (models/bonus3d.lua). The glowing
-- flag at the far end leads back to the courtyard's tower (spawn from_bonus).
local W = require( "World3D" )
local bonus = { name = "Bonus room" }
bonus.sfx = { soundfont = "music/sfx.sf2", program = 120, sounds = { coin = 72, jump = 76, land = 55 } }
bonus.world3d = {
  level = "models/bonus3d.lua",
  sky = { 0.18, 0.16, 0.22 },
  fog = { 0.18, 0.16, 0.22, 12, 30 },
  light = { { 0.3, -1.0, -0.5 }, { 1.0, 0.85, 0.65 }, 0.45, true },
  spawns = { entry = { 0, 0.1, 2.5, 0 } },
  build = function()
    local spots = {}
    for i = 0, 7 do
      local a = i * math.pi / 4
      spots[#spots + 1] = { math.cos( a ) * 4.5, 0.6, math.sin( a ) * 4.5 }
    end
    spots[#spots + 1] = { 0, 3.6, -3 }   -- on the tall block
    Plat.spawnCoins( "bonus", spots )
    W.Body.new{ x = 0, y = 0, z = -6, model = "flag3d" }
    W.door{ x = 0, y = 0, z = -6, radius = 0.9, height = 2.5, to = "courtyard", spawn = "from_bonus" }
  end,
}
return bonus
