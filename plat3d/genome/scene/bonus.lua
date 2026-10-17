-- The bonus room: a ring of coins and a flag back to the courtyard, placed in Blender (plat3d.blend, Blender scene
-- "bonus" -> genome/level/bonus.lua).
return {
  name = "Bonus room",
  world3d = {
    level = "bonus",
    sky = { 0.18, 0.16, 0.22 },
    fog = { 0.18, 0.16, 0.22, 12, 30 },
    light = { { 0.3, -1.0, -0.5 }, { 1.0, 0.85, 0.65 }, 0.45, true },
  },
}
