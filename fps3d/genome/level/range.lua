-- written by tools/blender (bicycle add-on) from fps3d.blend, Blender scene "range": don't edit, re-export.
-- objects: the level's meshes; entities: genomes placed in it (genome/<name>.lua) with per-instance
-- parameters; spawns: { x, y, z, yaw } where doors and Scene.go put the player.
return {
  objects = {
    { name = "floor", mesh = "model/floor", role = "static" },
    { name = "ceiling_beam_a", mesh = "model/ceiling_beam_a", role = "decor" },
    { name = "wall_n", mesh = "model/wall_n", role = "static" },
    { name = "wall_s", mesh = "model/wall_s", role = "static" },
    { name = "wall_w", mesh = "model/wall_w", role = "static" },
    { name = "wall_e", mesh = "model/wall_e", role = "static" },
    { name = "crate0", mesh = "model/crate0", role = "static" },
    { name = "crate1", mesh = "model/crate1", role = "static" },
    { name = "crate2", mesh = "model/crate2", role = "static" },
    { name = "crate3", mesh = "model/crate3", role = "static" },
    { name = "crate4", mesh = "model/crate4", role = "static" },
    { name = "crate5", mesh = "model/crate5", role = "static" },
    { name = "pillar0", mesh = "model/pillar0", role = "static" },
    { name = "pillar1", mesh = "model/pillar1", role = "static" },
    { name = "pillar2", mesh = "model/pillar2", role = "static" },
    { name = "divider_a", mesh = "model/divider_a", role = "static" },
    { name = "divider_b", mesh = "model/divider_b", role = "static" },
    { name = "catwalk", mesh = "model/catwalk", role = "static" },
    { name = "catwalk_stairs", mesh = "model/catwalk_stairs", role = "static" },
  },
  entities = {
    { genome = "camera", camera = { mode = "firstPerson", target = "player" }, key = "camera", pos = { -3.75, 1.8, 12 } },
    { genome = "player", key = "player", pos = { -3.75, 0.2, 12 } },
    { genome = "target", key = "target1", pos = { -3, 0, -14 } },
    { genome = "target", key = "target2", pos = { 6, 0, -12 } },
    { genome = "target", key = "target3", pos = { -12, 0, -2 }, yaw = 90 },
    { genome = "target", key = "target4", pos = { 12, 0, 0 }, yaw = -90 },
    { genome = "target", key = "target5", pos = { 12.5, 2.3, -10 }, yaw = -90 },
    { genome = "target", key = "target6", pos = { -12, 0, 12 }, yaw = 90 },
    { genome = "target", key = "target7", pos = { -3, 0, -3 } },
    { genome = "target", key = "target8", pos = { 8, 0, 13 }, yaw = -180 },
  },
  spawns = { default = { -3.75, 0.2, 12, 0 } },
}
