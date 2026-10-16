-- A butterfly circling the villager (Kinematics ORBITAL around an entity; radius in rows, so the orbit is twice
-- as many columns wide and looks round on 2x1 cells). Decorative: no collider.
local butterfly = {
  sprite = "butterfly_sprite", initAnim = "flap",
  motion = { type = "orbital", around = "villager", radius = 5, speed = 100 },
}
return butterfly
