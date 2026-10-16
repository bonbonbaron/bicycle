-- An invisible door: a trigger zone that changes scenes when the hero walks into it (the door gene, Genome.lua).
-- Each scene sets where its doors lead on the instance:
--   { genome = "door", pos = { 62, 38 }, door = { to = "house", spawn = "entry" } }
-- on = "press" makes it wait for E/Space instead (the hero calls Scene.useDoor()).
local door = { door = { to = "intro", w = 8, h = 8, on = "touch" } }
return door
