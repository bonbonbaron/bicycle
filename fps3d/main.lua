-- fps3d: a first-person shooter demo on bicycle's true-3D layer.
-- Run: ./o-3d fps3d   (3D build). Mouse/right stick: look, WASD/left stick: walk (Shift: sprint),
-- Space/pad A: jump, left mouse/F/pad RT: shoot (hitscan), Esc: release the mouse, Q: quit.
-- The level is fps3d/blender/fps3d.blend (scripts/blender/make_level.py -- fps, then export.py).
package.path = package.path .. ";./scripts/?.lua"
require("bicycle")
local ffi = require("ffi")
local W = require("World3D")
local Input = require("Input3D")
local Sound = require("Sound")

gameName = "fps3d"
if ffi.C.renderMode() ~= 2 then error( "fps3d needs the 3D build (-DBICYCLE_RENDER=3D)" ) end

ffi.C.pushScene()
Sound.load( nil, { soundfont = "music/sfx.sf2", program = 120, sounds = { shot = 60, hit = 72, ping = 76 } } )
W.level( "./fps3d/genome/models/scene3d.lua" )
W.View.sky( 0.30, 0.34, 0.42 )
W.View.fog( 0.30, 0.34, 0.42, 18, 60 )
W.View.light( { 0.35, -1.0, 0.5 }, { 1.0, 0.95, 0.85 }, 0.42, true )
W.View.crosshair( true )

Input.bind( "jump", { "space", "pad:a" } )
Input.bind( "fire", { "mouse:left", "f", "pad:rt" } )
Input.bind( "sprint", { "shift", "pad:ls" } )
Input.bind( "release", { "escape" } )
Input.bindAxis( "moveX", { neg = { "a" }, pos = { "d" }, pad = "leftx" } )
Input.bindAxis( "moveY", { neg = { "s" }, pos = { "w" }, pad = "lefty", padScale = -1 } )
Input.bindAxis( "turn", { neg = { "l" }, pos = { "j" } } )
Input.enableGamepad()
Input.captureMouse( true )

local player = W.Body.new{ kind = "character", x = -3.75, y = 0.2, z = 12, radius = 0.35, height = 1.75, collType = 1,
                           jumpSpeed = 7.5, gravity = 22, accel = 80, airAccel = 15 }
W.Camera.firstPerson( player.id, { eye = 1.6, fov = 75, yaw = 0, pitch = 0, minPitch = -85, maxPitch = 85 } )
local gun = W.Body.new{ model = "gun3d", scale = 0.7 }
local score = 0
local hud = ffi.C.addHudBar( "Targets", 8, 10, 2, 1 )
ffi.C.setHudBar( hud, 0 )

-- Targets: a disc on a post; shot ones fall over (Kinematics doesn't own them: Lua animates the pitch) and stand
-- back up after a few seconds.
local targets = {}
local spots = { { -3, -14, 0 }, { 6, -12, 0 }, { -12, -2, 90 }, { 12, 0, -90 }, { 12.5, -10, -90 },
                { -12, 12, 90 }, { -3, -3, 0 }, { 8, 13, 180 } }
for i, s in ipairs( spots ) do
  local y = s[2] == -10 and 2.3 or 0  -- the one on the catwalk
  local t = W.Body.new{ kind = "kinematic", x = s[1], y = y, z = s[2], yaw = s[3], radius = 0.45, height = 2.0,
                        model = "target3d" }
  t.parts = { W.Body.new{ x = s[1], y = y, z = s[2], yaw = s[3], model = "post3d" },
              W.Body.new{ x = s[1], y = y, z = s[2], yaw = s[3], model = "bullseye3d" } }
  t.yaw, t.fall, t.down = s[3], 0, 0
  targets[t.id] = t
end
local function pose( t )
  for _, b in ipairs( { t, t.parts[1], t.parts[2] } ) do ffi.C.xfSetRot( b.id, t.yaw, -t.fall, 0 ) end
end
-- A hit spark that lingers a moment at the impact point.
local sparks, nextSpark = {}, 1
for i = 1, 6 do sparks[i] = W.Body.new{ model = "spark3d" } sparks[i]:show( false ) sparks[i].ttl = 0 end

function player:onInput( key )
  if key == ffi.C.Q then ffi.C.popWindow() end
end
register( player.id, player )
focusOn( player.id )

local recoil, cooldown = 0, 0
local function shoot()
  local ex, ey, ez = W.Camera.eye()
  local f = W.Camera.forward()
  local hit = W.raycast( { ex, ey, ez }, f, 100, player.id )
  W.sound( Sound.sounds.shot, 100, ex, ey, ez )
  recoil = 1
  if not hit then return end
  local s = sparks[nextSpark]
  nextSpark = nextSpark % #sparks + 1
  s:setPos( hit.x + hit.nx * 0.05, hit.y + hit.ny * 0.05 - 0.08, hit.z + hit.nz * 0.05 )
  s:show( true )
  s.ttl = 0.4
  local t = targets[hit.entity]
  if t and t.down == 0 then
    t.down = 4  -- seconds until it stands back up
    ffi.C.bodySet( t.id, 0, 0, 0, 0, 0, false )  -- no longer shootable while down
    score = score + 1
    ffi.C.setHudBar( hud, score )
    W.sound( Sound.sounds.hit, 120, hit.x, hit.y, hit.z )
  end
end

function onFrame( dt )
  if Input.pressed( "release" ) then Input.captureMouse( false ) end
  local dx, dy = Input.look( dt )
  W.Camera.look( -dx * 0.15 + Input.axis( "turn" ) * 120 * dt, -dy * 0.15 )
  local mx, my = Input.axis( "moveX" ), Input.axis( "moveY" )
  local len = math.sqrt( mx * mx + my * my )
  if len > 1 then mx, my = mx / len, my / len end
  local speed = Input.down( "sprint" ) and 7.5 or 4.5
  local vx, vz = W.Camera.relative( mx * speed, my * speed )
  player:wish( vx, vz )
  local yaw, pitch = W.Camera.angles()
  player:setYaw( yaw )
  if Input.pressed( "jump" ) and player:grounded() then player:jump() end
  cooldown = math.max( 0, cooldown - dt )
  if Input.down( "fire" ) and cooldown == 0 then shoot() cooldown = 0.18 end
  -- The gun: in front of the eye, turned with the view, kicked back by recoil.
  recoil = math.max( 0, recoil - dt * 8 )
  local ex, ey, ez = W.Camera.eye()
  local f = W.Camera.forward()
  local ry = math.rad( yaw )
  local rx, rz = math.cos( ry ), -math.sin( ry )
  local back = 0.34 - recoil * 0.06
  gun:setPos( ex + f[1] * back + rx * 0.2, ey + f[2] * back - 0.2, ez + f[3] * back + rz * 0.2 )
  ffi.C.xfSetRot( gun.id, yaw, pitch + recoil * 6, 0 )
  for _, t in pairs( targets ) do
    if t.down > 0 then
      t.fall = math.min( 90, t.fall + dt * 400 )
      t.down = t.down - dt
      if t.down <= 0 then
        t.down, t.fall = 0, 0
        ffi.C.bodySet( t.id, 2, 0.45, 2.0, 0, 0, false )
        local x, y, z = t:pos()
        W.sound( Sound.sounds.ping, 80, x, y + 1, z )
      end
      pose( t )
    end
  end
  for _, s in ipairs( sparks ) do
    if s.ttl > 0 then s.ttl = s.ttl - dt if s.ttl <= 0 then s:show( false ) end end
  end
  if os.getenv( "BICYCLE_DEMO_LOG" ) then
    frameNo = ( frameNo or 0 ) + 1
    if frameNo % 15 == 0 then
      local x, y, z = player:pos()
      io.stderr:write( string.format( "fps3d f%d player %.2f %.2f %.2f yaw %.1f pitch %.1f score %d\n", frameNo, x, y, z, yaw, pitch, score ) )
    end
  end
end
