require('bicycle')
-- A horse that wanders on a timer (its timer gene -> onTimer), moves with the motion system (Kinematics),
-- picks directions with dice, and kicks the hero (an "enemy" collider: blocks and hurts) with a neigh.
-- img is the hand-drawn ASCII horse; sprite2d is its pixel version (test/aseprite/make_horse.lua) for 2D/3D.
-- timer: every 1.5 s it picks a new direction (onTimer); sfx: its own neigh.
local horse = {
  img = "horse", sprite2d = "horse_sprite", initAnim = "walk", coll = { type = "enemy" },
  timer = { ms = 1500 },
  sfx = { soundfont = "meadow", program = 120, sounds = { neigh = 64 } },
}

local SPEED = 48  -- fixed point per frame (~5.6 cells/second)
local HERO = 1    -- Genome.COLL.hero
local HOME_RANGE = 25

function horse:onSpawn()
  local pos = ffi.C.World_getBoxes().arr[self.id].pos
  self.home = { x = pos.x, y = pos.y }
end

function horse:onTimer(timerId, timeoutType)
  local pos = ffi.C.World_getBoxes().arr[self.id].pos
  local dx = ( pos.x - self.home.x ) / 256
  -- Mostly amble left or right, sometimes stand still; head home when it strays too far.
  local roll = ffi.C.rollDice( 1, 3 )
  local vx = ( roll == 1 and -SPEED ) or ( roll == 2 and SPEED ) or 0
  if dx > HOME_RANGE then vx = -SPEED elseif dx < -HOME_RANGE then vx = SPEED end
  ffi.C.setVelocity( self.id, vx, 0 )
  self.vx = vx
  dbglog( "horse at %d,%d walks vx=%d", pos.x / 256, pos.y / 256, vx )
end

function horse:onCollision(other, collType)
  if collType == HERO then
    -- Stop, back off a little from where it was heading, and complain.
    ffi.C.setVelocity( self.id, 0, 0 )
    if self.vx and self.vx ~= 0 then
      local pos = ffi.C.World_getBoxes().arr[self.id].pos
      pos.x = pos.x - ( self.vx > 0 and 3 or -3 ) * 256
    end
    self.vx = 0
    self:sound( "neigh" )
    dbglog( "horse neighs" )
  end
end

function horse:onInput(input) end

return horse
