-- A beetle patrolling the ground (Kinematics LINEAR along waypoints, ping-pong). Harmless: no collider.
local beetle = {
  sprite = "beetle_sprite", initAnim = "crawl",
  motion = { type = "linear", speed = 40, path = { { 0, 0 },  { 30, 30 }, { -10, -30 }, { 5, 0 }, } },
}
return beetle
