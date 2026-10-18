-- The 3D camera as a genome: a game's genome/camera.lua is data (mode, target, distances...) and this gives it its
-- behavior. Place it in Blender like any genome (its target is a genome drop-down) or spawn it from a scene.
--   local camera = { camera = { mode = "orbit", target = "hero", track = "lerp", rate = 12, distance = 7, ... } }
--   require( "CameraGenome" ).mixin( camera )
--   return camera
-- camera fields (all optional but target):
--   mode     "orbit" (third person, mouse orbits), "firstPerson" (at the target's eye) or "side" (side-scroller)
--   target   the genome it tracks (its placed instance, or the persistent one carried between scenes)
--   track    "lerp" (eases toward the target at rate, 1/s) or "hard" (snaps every frame)
--   distance, height (orbit/side), eye (firstPerson eye height), yaw, pitch (start angles), minPitch, maxPitch, fov,
--   collide (orbit: pushed in by walls)
--   lookX, lookY   degrees per pixel of mouse look (the Look input); invertY
--   keyTurn, keyPitch   degrees per second while J/L or I/K are held
--   captureMouse   grab the mouse for look when spawned (default true); Escape lets go, a click grabs it again
-- It listens to every input event (listen = true) without taking the focus from the hero, and persists so the
-- next scene's camera is the same one (World3D re-applies the rig after a scene change).
local ffi = require( "ffi" )
local W = require( "World3D" )
local M = {}

local DEFAULTS = { mode = "orbit", track = "lerp", lookX = 0.25, lookY = 0.2, keyTurn = 120, keyPitch = 90,
                   captureMouse = true }
local RIGS = { orbit = W.Camera.orbit, firstPerson = W.Camera.firstPerson, side = W.Camera.side }
local TURN = { [ffi.C.J] = { 1, 0 }, [ffi.C.L] = { -1, 0 }, [ffi.C.I] = { 0, 1 }, [ffi.C.K] = { 0, -1 } }

local methods = {}

-- The target's entity id, once it exists.
function methods:targetId()
  local t = self.camera.target
  if type( t ) == "number" then return t end
  local b = W.persistentByName[t]
  if b and not b.removed then return b.id end
  local id = require( "Genome" ).byName[t]
  if id and entityOf( id ) then return id end
end

-- Points the rig at the target; false if the target isn't there (yet).
function methods:aim()
  local c = self.camera
  local id = self:targetId()
  if not id then return false end
  local rig = RIGS[c.mode] or error( "camera: unknown mode '"..tostring( c.mode ).."' (orbit, firstPerson or side)" )
  local opts = {}
  for k, v in pairs( c ) do opts[k] = v end
  opts.mode = c.track  -- the rig's "hard" (snap) or lerp at rate
  rig( id, opts )
  self.aimed = id
  return true
end

function methods:onSpawn()
  for k, v in pairs( DEFAULTS ) do if self.camera[k] == nil then self.camera[k] = v end end
  self.turnKeys = {}
  self:aim()  -- or, if the level places the target after us, in onLevelReady
  if self.camera.captureMouse then require( "Input3D" ).captureMouse( true ) end
end

function methods:onLevelReady()
  if not self.aimed and not self:aim() then error( "camera: no '"..tostring( self.camera.target ).."' to follow" ) end
end

-- Mouse look orbits (once a frame while the mouse moves), J/L/I/K turn while held, Escape lets go of the mouse.
function methods:onInput( key, pressed, dx, dy )
  local c = self.camera
  key = tonumber( key )
  if key == ffi.C.Look then
    W.Camera.look( -dx * c.lookX, ( c.invertY and dy or -dy ) * c.lookY )
    return
  end
  if TURN[key] then
    self.turnKeys[key] = pressed or nil
    local yaw, pitch = 0, 0
    for k in pairs( self.turnKeys ) do yaw, pitch = yaw + TURN[k][1], pitch + TURN[k][2] end
    W.Camera.turn( yaw * c.keyTurn, pitch * c.keyPitch )
    return
  end
  if not pressed then return end
  local Input = require( "Input3D" )
  if key == ffi.C.Escape then Input.captureMouse( false )
  elseif key == ffi.C.MouseLeft and c.captureMouse then Input.captureMouse( true ) end
end

-- Adds the camera behavior to genome table g (its own methods win) and makes it a persistent listener.
function M.mixin( g )
  g.camera = g.camera or {}
  if g.listen == nil then g.listen = true end
  if g.persist == nil then g.persist = true end
  for k, fn in pairs( methods ) do if g[k] == nil then g[k] = fn end end
  return g
end

return M
