-- plat3d: a third-person 3D platformer demo (Mario 64-style controls) on bicycle's true-3D layer.
-- Run: ./o-3d plat3d   (3D build). WASD/left stick: run (camera-relative), Space/pad A: jump,
-- mouse/right stick/J L I K: orbit the camera, Esc: release the mouse, Q: quit.
-- Scenes: genome/scene/courtyard.lua (start) and bonus.lua. Everything in them (coins, the moving platform, the
-- flags that are doors, the hero, spawn points) is a genome (genome/<name>.lua) placed in Blender:
-- plat3d/blender/plat3d.blend with the add-on in tools/blender (docs/blender-entities.md).
package.path = package.path .. ";./scripts/?.lua"
require("bicycle")
local ffi = require("ffi")
local W = require("World3D")
local Input = require("Input3D")
local Genome = require("Genome")

gameName = "plat3d"
if ffi.C.renderMode() ~= 2 then error( "plat3d needs the 3D build (-DBICYCLE_RENDER=3D)" ) end

local Scene = require("Scene")
Input.bind( "jump", { "space", "pad:a" } )
Input.bindAxis( "moveX", { neg = { "a" }, pos = { "d" }, pad = "leftx" } )
Input.bindAxis( "moveY", { neg = { "s" }, pos = { "w" }, pad = "lefty", padScale = -1 } )
Input.bindAxis( "camYaw", { neg = { "l" }, pos = { "j" } } )
Input.bindAxis( "camPitch", { neg = { "k" }, pos = { "i" } } )
Input.bind( "release", { "escape" } )
Input.enableGamepad()
Input.captureMouse( true )

Scene.start( os.getenv( "PLAT3D_SCENE" ) or "courtyard" )
local function spawnPoint()
  local s = Scene.world3d and Scene.world3d.spawns or {}
  return s.default or s.entry or { 0, 1, 0, 0 }
end
-- The courtyard's level places the hero; starting anywhere else, it stands on that scene's spawn point.
local hero = W.player or Genome.spawn3d( "hero", { pos = spawnPoint() } )

-- The game's controller. Running is analog (a stick's tilt, held keys, the camera's current yaw), so it's polled
-- every frame here rather than being a genome event; the hero genome only gets events (onInput, onCollision...).
local RUN = 7
local AUTO = tonumber( os.getenv( "PLAT3D_AUTOCOLLECT" ) or "" )
local yaw, wasGrounded, frameNo = 0, true, 0
function onFrame( dt )
  frameNo = frameNo + 1
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
  local grounded = hero:grounded()
  if Input.pressed( "jump" ) and grounded then
    hero:jump()
    hero:sound( "jump", 90 )
  end
  if grounded and not wasGrounded then hero:sound( "land", 70 ) end
  wasGrounded = grounded
  -- PLAT3D_AUTOCOLLECT=N (tests/coin_collect_test.sh): every N frames, put the hero on a coin still out there.
  if AUTO and frameNo % AUTO == 0 and not Scene.busy() then
    for _, b in ipairs( W.live ) do
      if b.genomeName == "coin" then local cx, cy, cz = b:pos() hero:setPos( cx, cy - 0.4, cz ) break end
    end
  end
  local x, y, z = hero:pos()
  if y < -15 then
    local sp = spawnPoint()
    hero:setPos( sp[1], sp[2] + 0.5, sp[3] ) hero:setVel( 0, 0, 0 )
  end
  if os.getenv( "BICYCLE_DEMO_LOG" ) and frameNo % 15 == 0 then
    local ex, ey, ez = W.Camera.eye()
    io.stderr:write( string.format( "plat3d f%d hero %.2f %.2f %.2f grounded %s coins %d eye %.2f %.2f %.2f tris %d\n", frameNo, x, y, z, tostring( grounded ), hero.coins, ex, ey, ez, tonumber( ffi.C.levelTriangles() ) ) )
  end
end
