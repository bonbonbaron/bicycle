-- The plat3d bonus room (genome/scene/bonus.lua). Hand-written boxes (no Blender needed): a stone floor, wooden
-- walls and three blocks to jump on.
return {
  objects = {
    { name = "bonus_floor", mesh = "model/bonus_floor", role = "static" },
    { name = "bonus_walls", mesh = "model/bonus_walls", role = "static" },
    { name = "bonus_blocks", mesh = "model/bonus_blocks", role = "static" },
  }
}
