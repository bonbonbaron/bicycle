-- A moving platform: a solid box the hero rides, going back and forth along its path (declarative motion). Its
-- path is per instance: in Blender, select it and press "Add path point" (the points are relative to it).
local mover = {
  model = "mover3d",
  body = { kind = "box", half = { 1.5, 0.25, 1.5 } },
  motion = { type = "linear", speed = 1.2, pingpong = true, path = { { 0, 0, 0 }, { 0, 3, 0 } } },
  params = { ["motion.path"] = "path", ["motion.speed"] = "number", ["motion.pingpong"] = "bool" },
}
return mover
