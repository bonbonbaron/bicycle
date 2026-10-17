-- A shooting-range target: a disc on a post. Shot, it falls over backward (a short timer-driven animation), lies
-- there downFor ms and stands back up with a ping. The player's gun calls target:onShot( hit ) on what it hits.
local ffi = require( "ffi" )
local target = {
  model = "target3d",
  body = { kind = "kinematic", radius = 0.45, height = 2.0 },  -- what the gun's raycast hits
  sfx = { soundfont = "sfx", program = 120, sounds = { hit = 72, ping = 76 } },
  downFor = 4000,
  params = { downFor = "int" },
}
local FALL, STAND = 1, 2  -- its timers (onTimer's type)
local FALL_MS, FALL_STEP = 33, 15  -- 15 degrees a frame (30 fps): down in 0.2 s

function target:onSpawn()
  self.facing, self.fall = self:yaw(), 0
end

-- Returns true if this shot knocked it down.
function target:onShot( hit )
  if self.fall > 0 then return false end
  self.fall = 0.01
  ffi.C.bodySet( self.id, 0, 0, 0, 0, 0, false )  -- not shootable while down
  self:sound( "hit", 120 )
  self.falling = self:every( FALL_MS, FALL )
  return true
end

function target:onTimer( id, ttype )
  if ttype == FALL then
    self.fall = math.min( 90, self.fall + FALL_STEP )
    ffi.C.xfSetRot( self.id, self.facing, -self.fall, 0 )
    if self.fall >= 90 then
      self:stopTimer( self.falling )
      self:after( self.downFor, STAND )
    end
  elseif ttype == STAND then
    self.fall = 0
    ffi.C.xfSetRot( self.id, self.facing, 0, 0 )
    ffi.C.bodySet( self.id, 2, 0.45, 2.0, 0, 0, false )
    local x, y, z = self:pos()
    require( "Sound" ).playFor( self, "ping", 80, x, y + 1, z )
  end
end

return target
