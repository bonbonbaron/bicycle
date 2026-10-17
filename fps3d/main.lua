-- fps3d: a first-person shooter demo on bicycle's true-3D layer.
-- Run: ./o-3d fps3d   (3D build). Mouse/right stick: look, WASD/left stick: walk (Shift: sprint),
-- Space/pad A: jump, left mouse/F/pad RT: shoot (hitscan), Esc: release the mouse, Q: quit.
-- The scene is genome/scene/range.lua; its targets, the player, the camera and the spawn point are genomes placed
-- in Blender (fps3d/blender/fps3d.blend, scripts/blender/make_level.py -- fps; docs/blender-entities.md). The
-- controls are the genomes' onInput (player.lua, camera.lua); there is no per-frame game code.
package.path = package.path .. ";./scripts/?.lua"
require("bicycle")
local ffi = require("ffi")

gameName = "fps3d"
if ffi.C.renderMode() ~= 2 then error( "fps3d needs the 3D build (-DBICYCLE_RENDER=3D)" ) end
require("Input3D").enableGamepad()
require("Scene").start( "range" )
require("World3D").View.crosshair( true )
