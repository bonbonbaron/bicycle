-- plat3d: a third-person 3D platformer demo (Mario 64-style controls) on bicycle's true-3D layer.
-- Run: ./o-3d plat3d   (3D build). WASD/left stick: run (camera-relative), Space/pad A: jump,
-- mouse/right stick/J L I K: orbit the camera, Esc: release the mouse, Q: quit.
-- The level is plat3d/blender/plat3d.blend, made by scripts/blender/make_level.py and exported with
-- scripts/blender/export.py (see README there).
package.path = package.path .. ";./scripts/?.lua"
require("bicycle")
local ffi = require("ffi")
local W = require("World3D")
local Input = require("Input3D")
local Sound = require("Sound")

gameName = "plat3d"
if ffi.C.renderMode() ~= 2 then error( "plat3d needs the 3D build (-DBICYCLE_RENDER=3D)" ) end

ffi.C.pushScene()
Sound.load( nil, { soundfont = "music/sfx.sf2", program = 120, sounds = { coin = 72, jump = 76, land = 55 } } )
W.level( "./plat3d/genome/models/scene3d.lua" )
W.View.sky( 0.50, 0.70, 0.95 )
W.View.fog( 0.62, 0.76, 0.95, 25, 70 )
W.View.light( { -0.45, -1.0, -0.35 }, { 1.0, 0.96, 0.88 }, 0.40, true )

Input.bind( "jump", { "space", "pad:a" } )
Input.bindAxis( "moveX", { neg = { "a" }, pos = { "d" }, pad = "leftx" } )
Input.bindAxis( "moveY", { neg = { "s" }, pos = { "w" }, pad = "lefty", padScale = -1 } )
Input.bindAxis( "camYaw", { neg = { "l" }, pos = { "j" } } )
Input.bindAxis( "camPitch", { neg = { "k" }, pos = { "i" } } )
Input.bind( "release", { "escape" } )
Input.enableGamepad()
Input.captureMouse( true )

-- The hero: a capsule character drawn as the rigged pilgrim (Blender model faces +X: modelYaw 90 -> faces -Z).
local hero = W.Body.new{ kind = "character", x = 0, y = 0.5, z = 8, radius = 0.35, height = 1.6, collType = 1,
                         model = "pilgrim3d", modelYaw = 90, jumpSpeed = 10, gravity = 28, accel = 70, airAccel = 22 }
local RUN = 7
local coins, collected = {}, 0
local hud

-- Coins along the route (on the platforms, the ramp, the stairs and the tower), spinning and bobbing.
local spots = { { 4.5, 1.6, -2.5 }, { 9, 2.6, -5.5 }, { 12.5, 3.6, -10.5 }, { 7.5, 4.8, -15.5 }, { -6, 1.2, 4 },
                { 0, 3.1, 4 }, { -9, 1.2, -6.5 }, { -6.5, 2.6, -6.5 }, { 0, 2.2, -10 }, { -14, 0.6, 14 } }
for _, p in ipairs( spots ) do
  local c = W.Body.new{ kind = "kinematic", x = p[1], y = p[2], z = p[3], radius = 0.4, height = 0.8, collType = 2,
                        model = "coin3d", pitch = 90 }
  c:orbit{ center = { p[1], p[2], p[3] }, radius = 0, speed = 0, bob = 0.12, bobHz = 0.8 }
  c:spin( 180 )
  coins[c.id] = c
end
hud = ffi.C.addHudBar( "Coins", #spots, 10, 2, 1 )
ffi.C.setHudBar( hud, 0 )
-- A moving platform between the ground and the tower (a kinematic box the hero is carried by).
local mover = W.Body.new{ kind = "box", x = 2, y = 0.75, z = -14, half = { 1.5, 0.25, 1.5 }, model = "mover3d" }
mover:path( { { 2, 0.75, -14 }, { 2, 4.0, -14 } }, 1.2, true )
W.Body.new{ x = 7.5, y = 4.2, z = -17.5, model = "flag3d" }  -- the goal

W.Camera.orbit( hero.id, { distance = 7, height = 1.5, rate = 12, yaw = 0, pitch = -18, fov = 65 } )

function hero:onCollision( other, type )
  local c = coins[other]
  if c then
    coins[other] = nil
    local x, y, z = c:pos()
    c:remove()
    collected = collected + 1
    ffi.C.setHudBar( hud, collected )
    W.sound( Sound.sounds.coin, 115, x, y, z )
  end
end
function hero:onInput( key )
  if key == ffi.C.Q then ffi.C.popWindow() end
end
register( hero.id, hero )
focusOn( hero.id )

local yaw = 0  -- the hero's facing (degrees), turned smoothly toward the run direction
function onFrame( dt )
  if Input.pressed( "release" ) then Input.captureMouse( false ) end
  -- Camera: mouse / right stick / keys orbit it (the rig, not this script, places it).
  local dx, dy = Input.look( dt )
  W.Camera.look( -dx * 0.25 + Input.axis( "camYaw" ) * 120 * dt, -dy * 0.2 + Input.axis( "camPitch" ) * 90 * dt )
  -- Run relative to the camera; turn to face the way we run.
  local mx, my = Input.axis( "moveX" ), Input.axis( "moveY" )
  local len = math.sqrt( mx * mx + my * my )
  if len > 1 then mx, my = mx / len, my / len end
  local vx, vz = W.Camera.relative( mx * RUN, my * RUN )
  hero:wish( vx, vz )
  if len > 0.1 then
    local want = math.deg( math.atan2( -vx, -vz ) )
    local d = ( want - yaw + 540 ) % 360 - 180
    yaw = yaw + d * math.min( 1, 14 * dt )
    hero:setYaw( yaw )
  end
  ffi.C.setModelAction( hero.id, len > 0.1 and "walk" or "idle" )
  if Input.pressed( "jump" ) and hero:grounded() then
    hero:jump()
    local x, y, z = hero:pos()
    W.sound( Sound.sounds.jump, 90, x, y, z )
  end
  local x, y, z = hero:pos()
  if y < -15 then hero:setPos( 0, 1, 8 ) hero:setVel( 0, 0, 0 ) end
  if os.getenv( "BICYCLE_DEMO_LOG" ) then
    frameNo = ( frameNo or 0 ) + 1
    if frameNo % 15 == 0 then
      io.stderr:write( string.format( "plat3d f%d hero %.2f %.2f %.2f grounded %s coins %d\n", frameNo, x, y, z, tostring( hero:grounded() ), collected ) )
    end
  end
end
