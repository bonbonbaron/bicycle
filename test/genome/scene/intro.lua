local intro = {}
intro.name = "Level 1"
intro.bg = "level1"
intro.fg = { 
  [1] = { 
    { genome = "horse", pos = {15, 18} },
    { genome = "hero", pos = {40, 20}, focus = true }
  }
}
-- TODO: Demonstrate that you can populate the bg and fg with onTimer() too.
return intro
