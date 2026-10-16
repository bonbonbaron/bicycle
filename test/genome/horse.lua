require('bicycle')
local Sound = require('Sound')
-- A horse that wanders on a timer (Timer -> onTimeout), moves with the motion system (Kinematics),
-- picks directions with dice, and blocks the hero (collision) with a neigh (sound effect).
local horse = { img="horse", coll={ type="solid" } }

local SPEED = 48  -- fixed point per frame (~5.6 cells/second)
local HERO = 1    -- Genome.COLL.hero
local HOME_RANGE = 25

function horse:onSpawn()
  local pos = ffi.C.World_getBoxes().arr[self.id].pos
  self.home = { x = pos.x, y = pos.y }
  self.timer = ffi.C.createTimer( 1500, self.id, 0, true, ffi.C.TO_BRIDGE )
end

function horse:onTimeout(timerId, timeoutType)
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
    Sound.play( "neigh" )
    dbglog( "horse neighs" )
  end
end

function horse:onInput(input) end

return horse
