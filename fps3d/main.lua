-- fps3d: a first-person shooter demo on bicycle's true-3D layer.
-- Run: ./o-3d fps3d   (3D build). Mouse/right stick: look, WASD/left stick: walk (Shift: sprint),
-- Space/pad A: jump, left mouse/F/pad RT: shoot (hitscan), Esc: release the mouse, Q: quit.
-- The scene is genome/scene/range.lua; its targets, the player and the spawn point are genomes placed in Blender
-- (fps3d/blender/fps3d.blend, scripts/blender/make_level.py -- fps; docs/blender-entities.md).
package.path = package.path .. ";./scripts/?.lua"
require("bicycle")
local ffi = require("ffi")
local W = require("World3D")
local Input = require("Input3D")
local Genome = require("Genome")

gameName = "fps3d"
if ffi.C.renderMode() ~= 2 then error( "fps3d needs the 3D build (-DBICYCLE_RENDER=3D)" ) end

local Scene = require("Scene")
Input.bind( "jump", { "space", "pad:a" } )
Input.bind( "fire", { "mouse:left", "f", "pad:rt" } )
Input.bind( "sprint", { "shift", "pad:ls" } )
Input.bind( "release", { "escape" } )
Input.bindAxis( "moveX", { neg = { "a" }, pos = { "d" }, pad = "leftx" } )
Input.bindAxis( "moveY", { neg = { "s" }, pos = { "w" }, pad = "lefty", padScale = -1 } )
Input.bindAxis( "turn", { neg = { "l" }, pos = { "j" } } )
Input.enableGamepad()
Input.captureMouse( true )

Scene.start( "range" )
W.View.crosshair( true )
local player = W.player or Genome.spawn3d( "player", { pos = Scene.world3d.spawns.default } )

-- The game's controller: mouse look, analog walking and holding the trigger are polled every frame (they're
-- continuous input, not events), and the gun is kept in front of the camera. Targets and sparks are genomes
-- with their own events (onShot, onTimer).
local recoil, cooldown, frameNo = 0, 0, 0
function onFrame( dt )
  frameNo = frameNo + 1
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
  if Input.down( "fire" ) and cooldown == 0 then player:shoot() recoil, cooldown = 1, 0.18 end
  -- The gun: in front of the eye, turned with the view, kicked back by recoil.
  recoil = math.max( 0, recoil - dt * 8 )
  local ex, ey, ez = W.Camera.eye()
  local f = W.Camera.forward()
  local ry = math.rad( yaw )
  local rx, rz = math.cos( ry ), -math.sin( ry )
  local back = 0.34 - recoil * 0.06
  player.gun:setPos( ex + f[1] * back + rx * 0.2, ey + f[2] * back - 0.2, ez + f[3] * back + rz * 0.2 )
  ffi.C.xfSetRot( player.gun.id, yaw, pitch + recoil * 6, 0 )
  if os.getenv( "BICYCLE_DEMO_LOG" ) and frameNo % 15 == 0 then
    local x, y, z = player:pos()
    io.stderr:write( string.format( "fps3d f%d player %.2f %.2f %.2f yaw %.1f pitch %.1f score %d\n", frameNo, x, y, z, yaw, pitch, player.score ) )
  end
end
