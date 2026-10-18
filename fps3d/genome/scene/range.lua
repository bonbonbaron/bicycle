-- fps3d's shooting range. The targets, the player and its spawn point are placed in Blender
-- (fps3d/blender/fps3d.blend, Blender scene "range" -> genome/level/range.lua).
return {
  name = "Range",
  world3d = {
    level = "range",
    sky = { 0.30, 0.34, 0.42 },
    fog = { 0.30, 0.34, 0.42, 18, 60 },
    light = { { 0.35, -1.0, 0.5 }, { 1.0, 0.95, 0.85 }, 0.42, true },
  },
}
