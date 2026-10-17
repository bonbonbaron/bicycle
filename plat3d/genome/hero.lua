-- The hero: a capsule character drawn as the rigged pilgrim (its Blender model faces +X: modelYaw 90 -> -Z).
-- persist + player: carried through doors to the next scene's spawn point.
-- Controls are events: WASD (or the left stick) pressed/released sets a camera-relative run on the body
-- (W.Body:move; the physics step keeps it relative to the camera as that turns, and turnRate turns the hero to face
-- the way it runs), Space jumps, Q quits. The camera is its own genome (camera.lua).
local ffi = require( "ffi" )
local Scene = require( "Scene" )
local RUN = { [ffi.C.W] = { 0, 1 }, [ffi.C.S] = { 0, -1 }, [ffi.C.A] = { -1, 0 }, [ffi.C.D] = { 1, 0 } }
local AUTO_TIMER, LOG_TIMER = 1, 2
local hero = {
  model = "pilgrim3d",
  body = { kind = "character", radius = 0.35, height = 1.6, modelYaw = 90, jumpSpeed = 10, gravity = 28,
           accel = 70, airAccel = 22, turnRate = 14 },
  coll = { type = "hero" },
  persist = true, player = true,
  sfx = { soundfont = "sfx", program = 120, sounds = { jump = 76, land = 55 } },
  speed = 7,                   -- m/s running
  killY = -15,                 -- below this: onFallOut
  coins = 0, coinsTotal = 18,  -- 9 in the courtyard, 9 in the bonus room
}

function hero:onSpawn()
  self.hud = require( "Hud" ).bar( "Coins", self.coinsTotal )
  self.hud:set( self.coins )
  self.runKeys = {}
  focusOn( self.id )
  -- PLAT3D_AUTOCOLLECT=N (tests/coin_collect_test.sh): every N frames, put the hero on a coin still out there.
  self.auto = tonumber( os.getenv( "PLAT3D_AUTOCOLLECT" ) or "" )
  if self.auto then self:every( self.auto * 1000 / 30, AUTO_TIMER ) end
  if os.getenv( "BICYCLE_DEMO_LOG" ) then self:every( 500, LOG_TIMER ) end
end

function hero:onInput( key, pressed )
  key = tonumber( key )
  if RUN[key] then
    self.runKeys[key] = pressed or nil
    local mx, my = 0, 0
    for k in pairs( self.runKeys ) do mx, my = mx + RUN[k][1], my + RUN[k][2] end
    local len = math.sqrt( mx * mx + my * my )
    if len > 1 then mx, my = mx / len, my / len end
    self:move( mx * self.speed, my * self.speed )
    ffi.C.setModelAction( self.id, len > 0 and "walk" or "idle" )
    return
  end
  if not pressed then return end
  if key == ffi.C.Space then
    if self:grounded() then self:sound( "jump", 90 ) end
    self:jump()  -- (a little after walking off a ledge still counts)
  elseif key == ffi.C.Q then
    ffi.C.popWindow()
  end
end

function hero:onLand() self:sound( "land", 70 ) end

-- Fell off the world: back to this scene's spawn point.
function hero:onFallOut()
  local s = Scene.world3d and Scene.world3d.spawns or {}
  local sp = s.default or s.entry or { 0, 1, 0 }
  self:setPos( sp[1], sp[2] + 0.5, sp[3] )
  self:setVel( 0, 0, 0 )
end

-- Coins call this when the hero touches them.
function hero:collect( coin )
  self.coins = self.coins + 1
  self.hud:set( self.coins )
  if self.auto then io.stderr:write( "plat3d coin "..self.coins.."\n" ) end
end

function hero:onTimer( id, kind )
  local W = require( "World3D" )
  if kind == AUTO_TIMER and not Scene.busy() then
    for _, b in ipairs( W.live ) do
      if b.genomeName == "coin" then local cx, cy, cz = b:pos() self:setPos( cx, cy - 0.4, cz ) break end
    end
  elseif kind == LOG_TIMER then
    local x, y, z = self:pos()
    local ex, ey, ez = W.Camera.eye()
    io.stderr:write( string.format( "plat3d hero %.2f %.2f %.2f grounded %s coins %d eye %.2f %.2f %.2f tris %d\n", x, y, z,
      tostring( self:grounded() ), self.coins, ex, ey, ez, tonumber( ffi.C.levelTriangles() ) ) )
  end
end

function hero:onSceneEnter( sceneName )
  if os.getenv( "BICYCLE_DEMO_LOG" ) then io.stderr:write( "plat3d entered "..sceneName.."\n" ) end
end

return hero
