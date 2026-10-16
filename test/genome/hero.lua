require('bicycle')
local Sound = require('Sound')
-- An animated character exported from Aseprite (see scripts/aseprite/sprite.lua).
local hero = { sprite="pilgrim", initAnim="walk_down", coll={ type="hero" } }

local STEP = 256  -- one cell in fixed-point (FIXEDPT_DEC_BITS = 8)
local DIRS = {
  [ffi.C.A] = { strip="walk_left",  dx=-2 * STEP, dy=0 },  -- 2 columns ~ 1 row, since cells are tall
  [ffi.C.D] = { strip="walk_right", dx= 2 * STEP, dy=0 },
  [ffi.C.W] = { strip="walk_up",    dx=0, dy=-STEP },
  [ffi.C.S] = { strip="walk_down",  dx=0, dy= STEP },
}
local SOLID = 2  -- Genome.COLL.solid

-- While touching something solid (e.g. it walked into us), refuse moves toward its center.
function hero:blockedBy(dir)
  local boxes = ffi.C.World_getBoxes()
  local me = boxes.arr[self.id]
  for other, _ in pairs( self.touching or {} ) do
    local b = boxes.arr[other]
    local dx = ( b.pos.x + b.size.w / 2 ) - ( me.pos.x + me.size.w / 2 )
    local dy = ( b.pos.y + b.size.h / 2 ) - ( me.pos.y + me.size.h / 2 )
    if dir.dx * dx + dir.dy * dy * 4 > 0 then return true end  -- rows are ~2x as tall as columns are wide
  end
  return false
end

function hero:onInput(input)
  local dir = DIRS[tonumber(input)]
  if dir and self:blockedBy( dir ) then
    self.anim:set( dir.strip )
    Sound.play( "bump" )
    dbglog( "hero blocked" )
  elseif dir then
    self.anim:set( dir.strip )  -- no-op if already walking this way
    local pos = ffi.C.World_getBoxes().arr[self.id].pos
    self.prevPos = { x = pos.x, y = pos.y }  -- where to step back to if this move bumps into something solid
    pos.x = pos.x + dir.dx
    pos.y = pos.y + dir.dy
    dbglog( "hero move to %d,%d", pos.x / STEP, pos.y / STEP )
  elseif input == ffi.C.Q then
    ffi.C.popWindow()
  end
end

-- Solid things block: step back to before the move that bumped into them.
function hero:onCollision(other, collType)
  if collType == SOLID then
    self.touching = self.touching or {}
    self.touching[other] = true
  end
  if collType == SOLID and self.prevPos then
    local pos = ffi.C.World_getBoxes().arr[self.id].pos
    pos.x, pos.y = self.prevPos.x, self.prevPos.y
    self.prevPos = nil
    Sound.play( "bump" )
    dbglog( "hero bumped; back to %d,%d", pos.x / STEP, pos.y / STEP )
  end
end

function hero:onUncollision(other, collType)
  if self.touching then self.touching[other] = nil end
end

return hero
