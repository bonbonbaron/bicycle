-- The first-person player: a capsule (the camera genome, camera.lua, sits at its eye) and a gun.
-- Controls are events: WASD (left stick) pressed/released sets a camera-relative walk on the body (W.Body:move),
-- Shift sprints, Space jumps, the left mouse button / F / pad RT shoots while held (a repeating timer), Q quits.
-- The gun rides in the camera's view (W.Body:follow{ view = true }) and recoils with W.Body:kick.
local ffi = require( "ffi" )
local W = require( "World3D" )
local WALK = { [ffi.C.W] = { 0, 1 }, [ffi.C.S] = { 0, -1 }, [ffi.C.A] = { -1, 0 }, [ffi.C.D] = { 1, 0 } }
local FIRE = { [ffi.C.MouseLeft] = true, [ffi.C.F] = true }
local FIRE_TIMER, COOL_TIMER, LOG_TIMER = 1, 2, 3
local player = {
  body = { kind = "character", radius = 0.35, height = 1.75, jumpSpeed = 7.5, gravity = 22, accel = 80, airAccel = 15 },
  coll = { type = "hero" },
  player = true,
  sfx = { soundfont = "sfx", program = 120, sounds = { shot = 60 } },
  walkSpeed = 4.5, sprintSpeed = 7.5,  -- m/s
  fireMs = 180,                        -- between shots while the trigger is held
  score = 0, targets = 8,
}

function player:onSpawn()
  self.gun = W.Body.new{ model = "gun3d", scale = 0.7 }
  self.gun:follow( W.Camera.id(), { view = true, offset = { 0.2, -0.2, 0.34 } } )  -- right, up, forward of the eye
  self.hud = ffi.C.addHudBar( "Targets", self.targets, 10, 2, 1 )
  ffi.C.setHudBar( self.hud, 0 )
  self.walkKeys, self.triggers = {}, {}
  focusOn( self.id )
  if os.getenv( "BICYCLE_DEMO_LOG" ) then self:every( 500, LOG_TIMER ) end
end

function player:walk()
  local mx, my = 0, 0
  for k in pairs( self.walkKeys ) do mx, my = mx + WALK[k][1], my + WALK[k][2] end
  local len = math.sqrt( mx * mx + my * my )
  if len > 1 then mx, my = mx / len, my / len end
  local speed = self.sprinting and self.sprintSpeed or self.walkSpeed
  self:move( mx * speed, my * speed )
end

function player:onInput( key, pressed )
  key = tonumber( key )
  if WALK[key] then
    self.walkKeys[key] = pressed or nil
    self:walk()
  elseif key == ffi.C.LeftShift or key == ffi.C.RightShift then
    self.sprinting = pressed
    self:walk()
  elseif FIRE[key] then
    self.triggers[key] = pressed or nil
    if next( self.triggers ) and not self.fireTimer then
      if not self.cooling then self:fire() end
      self.fireTimer = self:every( self.fireMs, FIRE_TIMER )
    elseif not next( self.triggers ) and self.fireTimer then
      self:stopTimer( self.fireTimer )
      self.fireTimer = nil
    end
  elseif pressed and key == ffi.C.Space then
    if self:grounded() then self:jump() end
  elseif pressed and key == ffi.C.Q then
    ffi.C.popWindow()
  end
end

function player:onTimer( id, kind )
  if kind == FIRE_TIMER then self:fire()
  elseif kind == COOL_TIMER then self.cooling = false
  elseif kind == LOG_TIMER then
    local x, y, z = self:pos()
    local yaw, pitch = W.Camera.angles()
    io.stderr:write( string.format( "fps3d player %.2f %.2f %.2f yaw %.1f pitch %.1f score %d\n", x, y, z, yaw, pitch, self.score ) )
  end
end

-- One shot: recoil, and no faster than fireMs even when tapping.
function player:fire()
  self:shoot()
  self.gun:kick( 0.06, 6, 8 )
  self.cooling = true
  self:after( self.fireMs, COOL_TIMER )
end

-- Shoots along the view: a spark where it hits, and whatever was hit gets onShot( hit ).
function player:shoot()
  local ex, ey, ez = W.Camera.eye()
  local hit = W.raycast( { ex, ey, ez }, W.Camera.forward(), 100, self.id )
  self:sound( "shot", 100 )
  if not hit then return end
  require( "Genome" ).spawn3d( "spark", { pos = { hit.x + hit.nx * 0.05, hit.y + hit.ny * 0.05 - 0.08, hit.z + hit.nz * 0.05 } } )
  local e = entityOf( hit.entity )
  if e and e.onShot and e:onShot( hit ) then
    self.score = self.score + 1
    ffi.C.setHudBar( self.hud, self.score )
  end
end

return player
