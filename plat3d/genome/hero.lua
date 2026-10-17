-- The hero: a capsule character drawn as the rigged pilgrim (its Blender model faces +X: modelYaw 90 -> -Z).
-- persist + player: carried through doors to the next scene's spawn point. Running and jumping are analog
-- controls, polled in main.lua (see there); everything else about the hero is here.
local ffi = require( "ffi" )
local hero = {
  model = "pilgrim3d",
  body = { kind = "character", radius = 0.35, height = 1.6, modelYaw = 90, jumpSpeed = 10, gravity = 28,
           accel = 70, airAccel = 22 },
  coll = { type = "hero" },
  persist = true, player = true,
  sfx = { soundfont = "sfx", program = 120, sounds = { jump = 76, land = 55 } },
  coins = 0, coinsTotal = 18,  -- 9 in the courtyard, 9 in the bonus room
  camera = { distance = 7, height = 1.5, rate = 12, yaw = 0, pitch = -18, fov = 65 },
}

function hero:onSpawn()
  self.hud = require( "Hud" ).bar( "Coins", self.coinsTotal )
  self.hud:set( self.coins )
  require( "World3D" ).Camera.orbit( self.id, self.camera )
  focusOn( self.id )
end

-- Coins call this when the hero touches them.
function hero:collect( coin )
  self.coins = self.coins + 1
  self.hud:set( self.coins )
  if os.getenv( "PLAT3D_AUTOCOLLECT" ) then io.stderr:write( "plat3d coin "..self.coins.."\n" ) end
end

function hero:onSceneEnter( sceneName )
  if os.getenv( "BICYCLE_DEMO_LOG" ) then io.stderr:write( "plat3d entered "..sceneName.."\n" ) end
end

function hero:onInput( key )
  if key == ffi.C.Q then ffi.C.popWindow() end
end

return hero
