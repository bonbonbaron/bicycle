-- A flag that's a door: touch it to go to another scene. Each placed flag picks where it leads (Blender: the
-- "door.to" and "door.spawn" drop-downs of the Bicycle panel).
local flag = {
  model = "flag3d",
  door = { to = "courtyard", spawn = "default", radius = 0.9, height = 2.5, on = "touch" },
  params = { ["door.to"] = "scene", ["door.spawn"] = "spawn:door.to", ["door.on"] = "enum:touch|press" },
}
return flag
