-- The motion combo: a butterfly looping in small circles while its circle's center drifts back and forth across
-- the meadow (Kinematics COMBO: ORBITAL around a center moving LINEARly). Decorative: no collider.
local moth = {
  sprite = "butterfly_sprite", initAnim = "flap",
  motion = { type = "combo", radius = 3, speed = 200, travel = 5, path = { { 0, 0 }, { 50, 0 } }, pingpong = true },
}
return moth
