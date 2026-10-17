-- The first-person player: a capsule with the camera at eye height and a gun. Walking, looking and shooting are
-- analog controls polled in main.lua (see there); the player's genome is its body, camera, gun and sounds.
local ffi = require( "ffi" )
local player = {
  body = { kind = "character", radius = 0.35, height = 1.75, jumpSpeed = 7.5, gravity = 22, accel = 80, airAccel = 15 },
  coll = { type = "hero" },
  player = true,
  sfx = { soundfont = "sfx", program = 120, sounds = { shot = 60 } },
  score = 0, targets = 8,
  camera = { eye = 1.6, fov = 75, yaw = 0, pitch = 0, minPitch = -85, maxPitch = 85 },
}

function player:onSpawn()
  local W = require( "World3D" )
  W.Camera.firstPerson( self.id, self.camera )
  self.gun = W.Body.new{ model = "gun3d", scale = 0.7 }
  self.hud = ffi.C.addHudBar( "Targets", self.targets, 10, 2, 1 )
  ffi.C.setHudBar( self.hud, 0 )
  focusOn( self.id )
end

-- Shoots along the view: a spark where it hits, and whatever was hit gets onShot( hit ).
function player:shoot()
  local W = require( "World3D" )
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

function player:onInput( key )
  if key == ffi.C.Q then ffi.C.popWindow() end
end

return player
