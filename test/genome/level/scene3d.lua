-- written by tools/blender (bicycle add-on) from meadow3d.blend, Blender scene "Scene": don't edit, re-export.
-- objects: the level's meshes; entities: genomes placed in it (genome/<name>.lua) with per-instance
-- parameters; spawns: { x, y, z, yaw } where doors and Scene.go put the player.
return {
  objects = {
    { name = "ground", mesh = "model/ground", role = "static" },
    { name = "crate", mesh = "model/crate", role = "static" },
  },
  entities = {
  },
  spawns = {},
}
