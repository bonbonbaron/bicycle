-- A fairy that follows the hero around (Kinematics FOLLOW_LERP): she eases toward a spot behind and above him,
-- framerate-independently, and bobs while she hovers. No collider: she never blocks, hurts or gets hurt.
-- Art: test/aseprite/make_fairy.lua (pixels + ASCII export; 3D draws her as a camera-facing billboard).
local fairy = {
  sprite = "fairy_sprite", initAnim = "flutter",
  motion = { type = "follow", target = "hero", mode = "lerp", rate = 3.5, offset = { 9, -5 }, bob = 0.6, bobHz = 1.1 },
}
return fairy
