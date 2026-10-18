-- plat3d's first scene: the courtyard. What's in it (coins, the moving platform, the flag that leads to the bonus
-- room, the hero, spawn points) is placed in Blender (plat3d/blender/plat3d.blend, Blender scene "courtyard") and
-- exported to genome/level/courtyard.lua; this file only sets the scene-wide look.
return {
  name = "Courtyard",
  world3d = {
    level = "courtyard",
    sky = { 0.50, 0.70, 0.95 },
    fog = { 0.62, 0.76, 0.95, 25, 70 },
    light = { { -0.45, -1.0, -0.35 }, { 1.0, 0.96, 0.88 }, 0.40, true },
  },
}
