-- plat3d's first scene: the courtyard (plat3d/blender/plat3d.blend). Coins, a moving platform, and a door at the
-- flag on the tower that leads to the bonus room (genome/scene/bonus.lua).
local W = require( "World3D" )
local courtyard = { name = "Courtyard" }
courtyard.sfx = { soundfont = "music/sfx.sf2", program = 120, sounds = { coin = 72, jump = 76, land = 55 } }
courtyard.world3d = {
  level = "level/scene3d.lua",
  sky = { 0.50, 0.70, 0.95 },
  fog = { 0.62, 0.76, 0.95, 25, 70 },
  light = { { -0.45, -1.0, -0.35 }, { 1.0, 0.96, 0.88 }, 0.40, true },
  spawns = { default = { 4.5, 0.5, 9, 0 }, from_bonus = { 7.5, 4.4, -15.2, 0 } },
  build = function()
    -- Coins along the route, spinning and bobbing; ones already picked up stay gone (Scene.state).
    local spots = { { 4.5, 1.6, -2.5 }, { 9, 2.6, -5.5 }, { 12.5, 3.6, -10.5 }, { -6, 1.2, 4 },
                    { 0, 3.1, 4 }, { -9, 1.2, -6.5 }, { -6.5, 2.6, -6.5 }, { 0, 2.2, -10 }, { -14, 0.6, 14 } }
    Plat.spawnCoins( "courtyard", spots )
    -- A moving platform between the ground and the tower (a kinematic box the hero is carried by).
    local mover = W.Body.new{ kind = "box", x = 2, y = 0.75, z = -14, half = { 1.5, 0.25, 1.5 }, model = "mover3d" }
    mover:path( { { 2, 0.75, -14 }, { 2, 4.0, -14 } }, 1.2, true )
    W.Body.new{ x = 7.5, y = 4.2, z = -17.5, model = "flag3d" }
    -- The door: touch the flag to go to the bonus room.
    W.door{ x = 7.5, y = 4.2, z = -17.5, radius = 0.9, height = 2.5, to = "bonus", spawn = "entry" }
  end,
}
return courtyard
