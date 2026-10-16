-- A beetle patrolling the ground (Kinematics LINEAR along waypoints, ping-pong). Harmless: no collider.
local beetle = {
  sprite = "beetle_sprite", initAnim = "crawl",
  motion = { type = "linear", speed = 6, path = { { 0, 0 }, { 36, 0 }, { 36, 3 }, { 0, 3 } } },
}
return beetle
