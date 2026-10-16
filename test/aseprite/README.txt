pilgrim.aseprite is authored by make_pilgrim.lua and exported to test/genome/pilgrim*.lua by
scripts/aseprite/sprite.lua, both with the Aseprite CLI in batch mode (from the repo root):

  aseprite -b --script test/aseprite/make_pilgrim.lua
  aseprite -b test/aseprite/pilgrim.aseprite --script scripts/aseprite/sprite.lua
elder.aseprite (the villager NPC, 2-frame "idle") is made the same way with make_elder.lua.
