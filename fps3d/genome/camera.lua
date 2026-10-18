-- The first-person camera: at the player's eye, turned by the mouse / right stick / J L (I K pitch). Placed in
-- Blender with the player as its target ("camera.target" drop-down). Behavior: scripts/CameraGenome.lua.
local camera = {
  camera = { mode = "firstPerson", target = "player", track = "hard", eye = 1.6, fov = 75, yaw = 0, pitch = 0,
             minPitch = -85, maxPitch = 85, lookX = 0.15, lookY = 0.15 },
  params = { ["camera.target"] = "genome", ["camera.mode"] = "enum:firstPerson|orbit|side",
             ["camera.eye"] = "number", ["camera.fov"] = "number", ["camera.lookX"] = "number",
             ["camera.lookY"] = "number" },
}
require( "CameraGenome" ).mixin( camera )
return camera
