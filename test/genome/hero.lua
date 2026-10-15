require('bicycle')
-- An animated character exported from Aseprite (see scripts/aseprite/sprite.lua).
local hero = { sprite="pilgrim", initAnim="walk_down" }

local STEP = 256  -- one cell in fixed-point (FIXEDPT_DEC_BITS = 8)
local DIRS = {
  [ffi.C.A] = { strip="walk_left",  dx=-2 * STEP, dy=0 },  -- 2 columns ~ 1 row, since cells are tall
  [ffi.C.D] = { strip="walk_right", dx= 2 * STEP, dy=0 },
  [ffi.C.W] = { strip="walk_up",    dx=0, dy=-STEP },
  [ffi.C.S] = { strip="walk_down",  dx=0, dy= STEP },
}

function hero:onInput(input)
  local dir = DIRS[tonumber(input)]
  if dir then
    self.anim:set( dir.strip )  -- no-op if already walking this way
    local pos = ffi.C.World_getBoxes().arr[self.id].pos
    pos.x = pos.x + dir.dx
    pos.y = pos.y + dir.dy
  elseif input == ffi.C.Q then
    ffi.C.popWindow()
  end
end

return hero
