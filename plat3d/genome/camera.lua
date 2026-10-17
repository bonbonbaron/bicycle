-- The camera: third person, orbiting the hero (mouse / right stick / J L I K turn it, Esc lets go of the mouse).
-- Placed in Blender like any genome; its target is a genome drop-down ("camera.target"). The behavior is
-- scripts/CameraGenome.lua; this is just the data. It persists, so it carries over to the bonus room.
local camera = {
  camera = { mode = "orbit", target = "hero", track = "lerp", rate = 12, distance = 7, height = 1.5, yaw = 0,
             pitch = -18, fov = 65, lookX = 0.25, lookY = 0.2 },
  params = { ["camera.target"] = "genome", ["camera.mode"] = "enum:orbit|firstPerson|side",
             ["camera.track"] = "enum:lerp|hard", ["camera.rate"] = "number", ["camera.distance"] = "number",
             ["camera.height"] = "number", ["camera.pitch"] = "number", ["camera.fov"] = "number" },
}
require( "CameraGenome" ).mixin( camera )
return camera
