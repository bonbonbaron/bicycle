local intro = {}
intro.name = "Level 1"
-- An Aseprite background (test/aseprite/meadow.aseprite) with one layer per mode:
-- 1 sky (glued), 2 clouds (autoloop), 3 hills (parallax), 4 ground (fixed), 5 fence (loop).
-- The old text background is still at genome/scene/bg/level1 (set intro.bg = "level1" to use it).
intro.bg = "meadow"
intro.fg = { 
  [4] = { 
    { genome = "horse", pos = {15, 33} },
    { genome = "hero", pos = {40, 38}, focus = true }
  }
}
-- TODO: Demonstrate that you can populate the bg and fg with onTimer() too.
return intro
