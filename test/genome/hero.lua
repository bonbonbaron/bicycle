require('bicycle')
local Scene = require('Scene')
local Hud = require('Hud')
require('Dialogue')
-- The player: an animated Aseprite character (see scripts/aseprite/sprite.lua) who walks with WASD,
-- is blocked by solid things (coll.blockedBy), gets hurt by enemies (health bar up top), and talks to NPCs with E or Space.
-- model: in 3D mode the hero is the Blender character test/genome/model/pilgrim3d (scripts/blender/make_scene.py).
-- persist: the hero survives scene changes (Scene.go in scripts/Scene.lua): carried through doors, HUD and camera kept.
-- timer: the flash/invulnerability tick (onTimer); sfx: the hero's own sounds.
local TICK_MS = 100             -- flash/invulnerability timer period
local WALK_MS = 90              -- while a direction key is held: one step this often (the first on the press)
local WALK_DELAY_MS = 200       -- ...after this long held, so a quick tap is exactly one step
local WALK_TIMER, WALK_DELAY_TIMER = 7, 8  -- their timer types (the timer gene's is 0)
local hero = {
  sprite = "pilgrim", model = "pilgrim3d", initAnim = "walk_down", persist = true,
  coll = { type = "hero", blockedBy = { "solid", "enemy", "npc" } },  -- steps into these are undone by the engine
  timer = { ms = TICK_MS },
  sfx = { soundfont = "meadow", program = 120, sounds = { bump = 60, hurt = 55, blip = 76 } },
}

local STEP = 256  -- one cell in fixed-point (FIXEDPT_DEC_BITS = 8)
local DIRS = {
  [ffi.C.A] = { strip="walk_left",  dx=-2 * STEP, dy=0 },  -- 2 columns ~ 1 row, since cells are tall
  [ffi.C.D] = { strip="walk_right", dx= 2 * STEP, dy=0 },
  [ffi.C.W] = { strip="walk_up",    dx=0, dy=-STEP },
  [ffi.C.S] = { strip="walk_down",  dx=0, dy= STEP },
}
local SOLID, ENEMY, NPC = 2, 4, 5  -- Genome.COLL
local MAX_HEALTH = 10
local DAMAGE = 2
local KNOCKBACK = 6 * STEP      -- columns pushed away from whatever hurt us
local INVULNERABLE_TICKS = 10   -- ~1 s of invulnerability (and flashing) after a hit
local TALK_REACH = 2            -- cells of gap allowed between us and someone we talk to

local function box( id ) return ffi.C.World_getBoxes().arr[id] end

function hero:onSpawn()
  local pos = box( self.id ).pos
  self.spawn = { x = pos.x, y = pos.y }
  self.health = MAX_HEALTH
  self.hp = Hud.bar( "HP", MAX_HEALTH )
  self.invulnerable = 0
  -- The camera is a kinematic body tracking us (Camera.track in scripts/bicycle.lua). It glides after us with a
  -- dead zone; BICYCLE_CAMERA=hard makes it hard-track instead (snap, no dead zone).
  if os.getenv( "BICYCLE_CAMERA" ) == "hard" then
    Camera.track( self.id, { mode = "hard" } )
  else
    Camera.track( self.id, { mode = "lerp", rate = 5, deadzone = { 10, 3 } } )
  end
end

-- The speaker whose box is within TALK_REACH cells of ours (touching counts), if any.
function hero:speakerInReach()
  local me = box( self.id )
  local reach = TALK_REACH * STEP
  for _, npc in ipairs( Dialogue.speakers ) do
    local b = box( npc.id )
    local gapX = math.max( b.pos.x - ( me.pos.x + me.size.w ), me.pos.x - ( b.pos.x + b.size.w ) )
    local gapY = math.max( b.pos.y - ( me.pos.y + me.size.h ), me.pos.y - ( b.pos.y + b.size.h ) )
    if gapX <= reach and gapY <= reach then return npc end
  end
end

-- Input: a direction key steps once when pressed and, held past WALK_DELAY_MS, keeps stepping (a walk timer) until
-- it's released; the newest held direction wins. Releases of other keys mean nothing here.
function hero:onInput(input, pressed)
  input = tonumber( input )
  if DIRS[input] then
    self.held = self.held or {}
    for i = #self.held, 1, -1 do if self.held[i] == input then table.remove( self.held, i ) end end
    if pressed then table.insert( self.held, input ) end
    if #self.held == 0 and self.walkTimer then
      self:stopTimer( self.walkTimer )
      self.walkTimer = nil
    elseif #self.held > 0 and not self.walkTimer then
      self.walkTimer = self:after( WALK_DELAY_MS, WALK_DELAY_TIMER )
    end
    if pressed then self:step( DIRS[input] ) end
    return
  end
  if not pressed then return end
  if input == ffi.C.Q then
    ffi.C.popWindow()
    return
  end
  -- While a Dialogue is up it has the input focus, so we don't move (or get here at all).
  if input == ffi.C.E or input == ffi.C.Space then
    if Scene.useDoor() then return end  -- standing in a door (on = "press" doors wait for this)
    local npc = self:speakerInReach()
    if npc then
      self:sound( "blip" )
      dbglog( "talk to %s", npc.name )
      Dialogue.new( npc.name..":\n"..table.concat( npc.lines, "\n\n" ), nil, nil, nil, nil, function() dbglog( "talk done" ) end )
    end
    return
  end
end

function hero:step( dir )
  self.anim:set( dir.strip )  -- no-op if already walking this way
  if ffi.C.stepEntity( self.id, dir.dx, dir.dy ) then
    local pos = box( self.id ).pos
    dbglog( "hero move to %d,%d", pos.x / STEP, pos.y / STEP )
  else  -- up against something solid that way
    self:sound( "bump" )
    dbglog( "hero blocked" )
  end
end

function hero:hurt( by )
  if self.invulnerable > 0 or self.health <= 0 then
    dbglog( "hero hit while invulnerable; no damage" )
    return
  end
  self.health = self.hp:set( self.health - DAMAGE )
  self.invulnerable = INVULNERABLE_TICKS
  self:sound( "hurt" )
  -- Knock back, away from whatever hit us.
  local me, b = box( self.id ), box( by )
  local away = ( me.pos.x + me.size.w / 2 ) < ( b.pos.x + b.size.w / 2 ) and -1 or 1
  ffi.C.teleport( self.id, me.pos.x + away * KNOCKBACK, me.pos.y )
  dbglog( "hero hurt: health %d", self.health )
  if self.health <= 0 then
    dbglog( "game over" )
    Dialogue.new( "You collapse in the tall grass...\n\n...and wake at the edge of the meadow, rested. Press E to try again.",
      nil, nil, nil, nil, function() self:respawn() end )
  end
end

function hero:respawn()
  ffi.C.teleport( self.id, self.spawn.x, self.spawn.y )
  self.health = self.hp:set( MAX_HEALTH )
  self.invulnerable = INVULNERABLE_TICKS
  dbglog( "respawn: health %d", self.health )
end

-- After walking through a door: we're at the new scene's spawn point.
function hero:onSceneEnter( sceneName, spawnName )
  local pos = box( self.id ).pos
  self.spawn = { x = pos.x, y = pos.y }
  dbglog( "hero entered %s (spawn %s)", sceneName, tostring( spawnName ) )
end

-- The walk timer steps in the newest held direction; the timer gene's tick flashes while invulnerable.
function hero:onTimer(timerId, timeoutType)
  if timeoutType == WALK_DELAY_TIMER then  -- held past a tap: walk on
    if tonumber( self.walkTimer ) ~= tonumber( timerId ) then return end  -- (released and pressed again meanwhile: a newer timer counts)
    self.walkTimer = self:every( WALK_MS, WALK_TIMER )
    timerId, timeoutType = self.walkTimer, WALK_TIMER
  end
  if timeoutType == WALK_TIMER then
    local held = self.held or {}
    if getFocus() ~= self.id then  -- a dialogue took the input (and will get the key's release): stop walking
      self.held = {}
      self:stopTimer( timerId )
      self.walkTimer = nil
    elseif #held > 0 then
      self:step( DIRS[held[#held]] )
    end
    return
  end
  if self.invulnerable > 0 then
    self.invulnerable = self.invulnerable - 1
    ffi.C.setHidden( self.id, self.invulnerable % 2 == 1 )
  end
end

function hero:onCollision(other, collType)
  if collType == ENEMY then
    self:hurt( other )
  elseif ( collType == SOLID or collType == NPC ) and ffi.C.stepBlocked( self.id ) then  -- walked into it: put back
    self:sound( "bump" )
    local pos = box( self.id ).pos
    dbglog( "hero bumped; back to %d,%d", pos.x / STEP, pos.y / STEP )
  end
end

return hero
