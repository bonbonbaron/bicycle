-- The plat3d bonus room (genome/scene/bonus.lua). Hand-written boxes (no Blender needed): a stone floor, wooden
-- walls and three blocks to jump on.
return {
  objects = {
    { name = "bonus_floor", mesh = "models/bonus_floor", role = "static" },
    { name = "bonus_walls", mesh = "models/bonus_walls", role = "static" },
    { name = "bonus_blocks", mesh = "models/bonus_blocks", role = "static" },
  }
}
