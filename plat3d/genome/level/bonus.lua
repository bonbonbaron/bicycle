-- written by tools/blender (bicycle add-on) from plat3d.blend, Blender scene "bonus": don't edit, re-export.
-- objects: the level's meshes; entities: genomes placed in it (genome/<name>.lua) with per-instance
-- parameters; spawns: { x, y, z, yaw } where doors and Scene.go put the player.
return {
  objects = {
    { name = "bonus_floor", mesh = "model/bonus_floor", role = "static" },
    { name = "bonus_walls", mesh = "model/bonus_walls", role = "static" },
    { name = "bonus_blocks", mesh = "model/bonus_blocks", role = "static" },
  },
  entities = {
    { genome = "coin", key = "bonus_coin1", pos = { 4.5, 0.6, 0 } },
    { genome = "coin", key = "bonus_coin2", pos = { 3.182, 0.6, 3.182 } },
    { genome = "coin", key = "bonus_coin3", pos = { 0, 0.6, 4.5 } },
    { genome = "coin", key = "bonus_coin4", pos = { -3.182, 0.6, 3.182 } },
    { genome = "coin", key = "bonus_coin5", pos = { -4.5, 0.6, 0 } },
    { genome = "coin", key = "bonus_coin6", pos = { -3.182, 0.6, -3.182 } },
    { genome = "coin", key = "bonus_coin7", pos = { 0, 0.6, -4.5 } },
    { genome = "coin", key = "bonus_coin8", pos = { 3.182, 0.6, -3.182 } },
    { genome = "coin", key = "bonus_coin9", pos = { 0, 3.6, -3 } },
    { genome = "camera", camera = { mode = "orbit", target = "hero", track = "lerp" }, key = "camera.001", pos = { 0, 3, 9 } },
    { genome = "flag", door = { on = "touch", spawn = "from_bonus", to = "courtyard" }, key = "flag_to_courtyard", pos = { 0, 0, -6 } },
    { genome = "hero", key = "hero.001", pos = { 0, 0.1, 2.5 } },
  },
  spawns = { entry = { 0, 0.1, 2.5, 0 } },
}
