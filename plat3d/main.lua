-- plat3d: a third-person 3D platformer demo (Mario 64-style controls) on bicycle's true-3D layer.
-- Run: ./o-3d plat3d   (3D build). WASD/left stick: run (camera-relative), Space/pad A: jump,
-- mouse/right stick/J L I K: orbit the camera, Esc: release the mouse, Q: quit.
-- Scenes: genome/scene/courtyard.lua (start) and bonus.lua. Everything in them (the hero, the camera, coins, the
-- moving platform, the flags that are doors, spawn points) is a genome (genome/<name>.lua) placed in Blender:
-- plat3d/blender/plat3d.blend with the add-on in tools/blender (docs/blender-entities.md). The controls are the
-- genomes' onInput (hero.lua, camera.lua); there is no per-frame game code.
package.path = package.path .. ";./scripts/?.lua"
require("bicycle")
local ffi = require("ffi")

gameName = "plat3d"
if ffi.C.renderMode() ~= 2 then error( "plat3d needs the 3D build (-DBICYCLE_RENDER=3D)" ) end
require("Input3D").enableGamepad()
require("Scene").start( os.getenv( "PLAT3D_SCENE" ) or "courtyard" )
