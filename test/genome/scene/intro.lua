local intro = {}
intro.name = "Level 1"
-- An Aseprite background (test/aseprite/meadow.aseprite) with one layer per mode:
-- 1 sky (glued), 2 clouds (autoloop), 3 hills (parallax), 4 ground (fixed), 5 fence (loop).
-- The old text background is still at genome/scene/bg/level1 (set intro.bg = "level1" to use it).
intro.bg = "meadow"
-- Where Scene.go( "intro", { spawn = ... } ) puts the hero (genome/scene/house.lua's door uses from_house).
intro.spawns = { default = {40, 38}, from_house = {54, 38} }
-- Entities: placed on meadow.aseprite's entities layer (scripts/aseprite/addentity.lua); the cottage door is a
-- door slice there too (walk into it, or press E, to go inside: genome/scene/house.lua). Motion demos (the motion
-- gene in scripts/Genome.lua): a fairy following the hero, a butterfly circling the villager, a moth looping while
-- drifting across, a patrolling beetle and its red palette swap (fire collision, genome/redbeetle.lua).
intro.fg = { 
  [4] = { 
    -- BEGIN entities (generated from the entities layer of meadow.aseprite: place them there)
    { genome = "cottage", pos = { 52, 26 } },
    { genome = "door", pos = { 64, 40 }, door = { h = 4, spawn = "entry", to = "house", w = 6 } },
    { genome = "horse", pos = { 110, 40 } },
    { genome = "coin", pos = { 20, 42 } },
    { genome = "coin", pos = { 64, 43 } },
    { genome = "coin", pos = { 140, 42 } },
    { genome = "villager", pos = { 2, 38 } },
    { genome = "hero", pos = { 40, 38 }, focus = true, persist = true },
    { genome = "fairy", pos = { 30, 33 }, persist = true },
    { genome = "butterfly", pos = { 10, 34 } },
    { genome = "moth", pos = { 100, 30 } },
    { genome = "beetle", pos = { 150, 45 } },
    { genome = "redbeetle", pos = { 50, 45 } },
    -- END entities
  }
}
-- Music (bare names: genome/music/meadow.sf2 and meadow.mid; see tools/audio/README.md for making these from
-- FL Studio, LMMS or midish). Sound effects aren't the scene's: each genome declares its own (its sfx gene).
intro.music = { soundfont = "meadow", song = "meadow" }
-- TODO: Demonstrate that you can populate the bg and fg with onTimer() too.
return intro
