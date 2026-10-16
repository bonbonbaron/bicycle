-- plat3d: a third-person 3D platformer demo (Mario 64-style controls) on bicycle's true-3D layer.
-- Run: ./o-3d plat3d   (3D build). WASD/left stick: run (camera-relative), Space/pad A: jump,
-- mouse/right stick/J L I K: orbit the camera, Esc: release the mouse, Q: quit.
-- Scenes: genome/scene/courtyard.lua (start) and bonus.lua; touch the flag on the tower to go to the bonus room,
-- and its flag to come back (Scene.go via W.door; the hero is a persistent body).
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

local Scene = require("Scene")
local Hud = require("Hud")
Input.bind( "jump", { "space", "pad:a" } )
Input.bindAxis( "moveX", { neg = { "a" }, pos = { "d" }, pad = "leftx" } )
Input.bindAxis( "moveY", { neg = { "s" }, pos = { "w" }, pad = "lefty", padScale = -1 } )
Input.bindAxis( "camYaw", { neg = { "l" }, pos = { "j" } } )
Input.bindAxis( "camPitch", { neg = { "k" }, pos = { "i" } } )
Input.bind( "release", { "escape" } )
Input.enableGamepad()
Input.captureMouse( true )

local RUN = 7
local coins, collected = {}, 0
local TOTAL = 18  -- 9 in the courtyard, 9 in the bonus room
local hud

-- Scenes call this from their build(): coins at spots, minus the ones already picked up there.
Plat = {}
function Plat.spawnCoins( sceneName, spots )
  local taken = Scene.stateOf( sceneName )
  for i, p in ipairs( spots ) do
    if not taken[i] then
      local c = W.Body.new{ kind = "kinematic", x = p[1], y = p[2], z = p[3], radius = 0.4, height = 0.8, collType = 2,
                            model = "coin3d", pitch = 90 }
      c:orbit{ center = { p[1], p[2], p[3] }, radius = 0, speed = 0, bob = 0.12, bobHz = 0.8 }
      c:spin( 180 )
      c.slot = i
      coins[c.id] = c
      c.onDespawn = function( self ) coins[self.id] = nil end
    end
  end
end

Scene.start( os.getenv( "PLAT3D_SCENE" ) or "courtyard" )
-- The hero: a capsule character drawn as the rigged pilgrim (Blender model faces +X: modelYaw 90 -> faces -Z).
-- persist + player: carried through doors to the next scene's spawn point (Scene.go in scripts/Scene.lua).
local hero = W.Body.new{ kind = "character", x = 4.5, y = 0.5, z = 9, radius = 0.35, height = 1.6, collType = 1,
                         model = "pilgrim3d", modelYaw = 90, jumpSpeed = 10, gravity = 28, accel = 70, airAccel = 22,
                         persist = true, player = true }
local sp = Scene.world3d.spawns.default or Scene.world3d.spawns.entry
hero:setPos( sp[1], sp[2], sp[3] )
hud = Hud.bar( "Coins", TOTAL )
hud:set( 0 )
W.Camera.orbit( hero.id, { distance = 7, height = 1.5, rate = 12, yaw = 0, pitch = -18, fov = 65 } )

function hero:onSceneEnter( sceneName )
  if os.getenv( "BICYCLE_DEMO_LOG" ) then io.stderr:write( "plat3d entered "..sceneName.."\n" ) end
end

function hero:onCollision( other, type )
  local c = coins[other]
  if c then
    coins[other] = nil
    local x, y, z = c:pos()
    Scene.state[c.slot] = true  -- stays picked up when we come back
    c:remove()
    collected = collected + 1
    hud:set( collected )
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
  if y < -15 then
    local sp = ( Scene.world3d and Scene.world3d.spawns or {} ).default or { 4.5, 1, 9 }
    hero:setPos( sp[1], sp[2] + 0.5, sp[3] ) hero:setVel( 0, 0, 0 )
  end
  if os.getenv( "BICYCLE_DEMO_LOG" ) then
    frameNo = ( frameNo or 0 ) + 1
    local ex, ey, ez = W.Camera.eye()
    if frameNo % 15 == 0 then
      io.stderr:write( string.format( "plat3d f%d hero %.2f %.2f %.2f grounded %s coins %d eye %.2f %.2f %.2f tris %d\n", frameNo, x, y, z, tostring( hero:grounded() ), collected, ex, ey, ez, tonumber( ffi.C.levelTriangles() ) ) )
    end
  end
end
