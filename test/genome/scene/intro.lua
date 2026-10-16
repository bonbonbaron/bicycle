local intro = {}
intro.name = "Level 1"
-- An Aseprite background (test/aseprite/meadow.aseprite) with one layer per mode:
-- 1 sky (glued), 2 clouds (autoloop), 3 hills (parallax), 4 ground (fixed), 5 fence (loop).
-- The old text background is still at genome/scene/bg/level1 (set intro.bg = "level1" to use it).
intro.bg = "meadow"
-- Where Scene.go( "intro", { spawn = ... } ) puts the hero (genome/scene/house.lua's door uses from_house).
intro.spawns = { default = {40, 38}, from_house = {54, 38} }
intro.fg = { 
  [4] = { 
    { genome = "cottage", pos = {52, 26} },
    -- The cottage door: walk into it (or press E there) to go inside (genome/scene/house.lua).
    { genome = "door", pos = {64, 40}, door = { to = "house", spawn = "entry", w = 6, h = 4 } },
    { genome = "horse", pos = {110, 40} },
    { genome = "coin", pos = {20, 42} },
    { genome = "coin", pos = {64, 43} },
    { genome = "coin", pos = {140, 42} },
    { genome = "villager", pos = {2, 38} },
    { genome = "hero", pos = {40, 38}, focus = true, persist = true },
    -- Motion demos (see the motion gene in scripts/Genome.lua): a fairy following the hero (follow), a butterfly
    -- circling the villager (orbital), one looping while drifting across (combo), a patrolling beetle (linear).
    { genome = "fairy", pos = {30, 33}, persist = true },
    { genome = "butterfly", pos = {10, 34} },
    { genome = "moth", pos = {100, 30} },
    { genome = "beetle", pos = {150, 45} }
  }
}
-- Music and sound effects (see tools/audio/README.md for making these from FL Studio, LMMS or midish).
intro.music = { soundfont = "music/meadow.sf2", song = "music/meadow.mid" }
intro.sfx = { soundfont = "music/meadow.sf2", program = 120, sounds = { bump = 60, neigh = 64, coin = 72, hurt = 55, blip = 76 } }
-- TODO: Demonstrate that you can populate the bg and fg with onTimer() too.
return intro
