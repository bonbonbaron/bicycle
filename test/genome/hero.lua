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
  sprite = "pilgrim", initAnim = "walk_down", persist = true,
  coll = { type = "hero", blockedBy = { "solid", "enemy", "npc" } },  -- the engine stops its steps flush against these
  timer = { ms = TICK_MS },
  vel = {x = 0, y = 0, z = 0},
  sfx = {
	  soundfont = "meadow",
	  program = 120,
	  sounds = {
		  bump = 60,
		  hurt = 55,
		  blip = 76
	  }
  },
}

hero.STEP = 256  -- one cell in fixed-point (FIXEDPT_DEC_BITS = 8)
hero.stepSize = 3
function hero:actOnInput(input)
  if input == ffi.C.A then
     self.anim:set( "walk_left" )
     ffi.C.setVelX(self.id, -self.stepSize * self.STEP)  -- TODO add velX/Y/Z setters so you don't have to set untouched components to themselves
    elseif input == ffi.C.D then
      self.anim:set( "walk_right" )
      ffi.C.setVelY(self.id, self.stepSize * self.STEP)  -- TODO add velX/Y/Z setters so you don't have to set untouched components to themselves
    elseif input == ffi.C.W then
      self.anim:set( "walk_up" )
      ffi.C.setVelZ(self.id, -self.STEP * self.stepSize)
    elseif input == ffi.C.S then
      self.anim:set( "walk_down" )
      ffi.C.setVelZ(self.id, self.STEP * self.stepSize)
    elseif input == ffi.C.LeftShift then
      self.stepSize = 6
    end
  end

function hero:actOffInput(input)
  print("hero id: "..self.id)
  if input == ffi.C.A then
     ffi.C.setVelX(self.id, 0)  -- TODO add velX/Y/Z setters so you don't have to set untouched components to themselves
    elseif input == ffi.C.D then
      ffi.C.setVelY(self.id, 0)  -- TODO add velX/Y/Z setters so you don't have to set untouched components to themselves
    elseif input == ffi.C.W then
      ffi.C.setVelZ(self.id, 0)
    elseif input == ffi.C.S then
      ffi.C.setVelZ(self.id, 0)
    elseif input == ffi.C.LeftShift then
      self.stepSize = 3
    end
  end

local SOLID, ENEMY, NPC = 2, 4, 5  -- Genome.COLL
hero.MAX_HEALTH = 10
hero.DAMAGE = 2
hero.KNOCKBACK = 6 * hero.STEP      -- columns pushed away from whatever hurt us
hero.INVULNERABLE_TICKS = 10   -- ~1 s of invulnerability (and flashing) after a hit
hero.TALK_REACH = 2            -- cells of gap allowed between us and someone we talk to

local function box( id ) return ffi.C.World_getBoxes().arr[id] end

function hero:onSpawn()
  local pos = box( self.id ).pos
  self.spawn = { x = pos.x, y = pos.y }
  self.health = self.MAX_HEALTH
  self.hp = Hud.bar( "HP", self.MAX_HEALTH )
  self.invulnerable = 0
  -- The camera is a kinematic body tracking us (Camera.track in scripts/bicycle.lua). It glides after us with a
  -- dead zone; BICYCLE_CAMERA=hard makes it hard-track instead (snap, no dead zone).
    Camera.track( self.id, { mode = "lerp", rate = 5, deadzone = { 10, 3 } } )
end

-- The speaker whose box is within TALK_REACH cells of ours (touching counts), if any.
function hero:speakerInReach()
  local me = box( self.id )
  local reach = self.TALK_REACH * self.STEP
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
  if pressed then
    self:actOnInput(input)
  else
    self:actOffInput(input)
  end
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

function hero:hurt( by )
  if self.invulnerable > 0 or self.health <= 0 then
    dbglog( "hero hit while invulnerable; no damage" )
    return
  end
  self.health = self.hp:set( self.health - self.DAMAGE )
  self.invulnerable = self.INVULNERABLE_TICKS
  self:sound( "hurt" )
  -- Knock back, away from whatever hit us.
  local me, b = box( self.id ), box( by )
  local away = ( me.pos.x + me.size.w / 2 ) < ( b.pos.x + b.size.w / 2 ) and -1 or 1
  ffi.C.teleport( self.id, me.pos.x + away * self.KNOCKBACK, me.pos.y )
  dbglog( "hero hurt: health %d", self.health )
  if self.health <= 0 then
    dbglog( "game over" )
    Dialogue.new( "You collapse in the tall grass...\n\n...and wake at the edge of the meadow, rested. Press E to try again.",
      nil, nil, nil, nil, function() self:respawn() end )
  end
end

function hero:respawn()
  ffi.C.teleport( self.id, self.spawn.x, self.spawn.y )
  self.health = self.hp:set( self.MAX_HEALTH )
  self.invulnerable = self.INVULNERABLE_TICKS
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
  if self.invulnerable > 0 then
    self.invulnerable = self.invulnerable - 1
    ffi.C.setHidden( self.id, self.invulnerable % 2 == 1 )
  end
end

function hero:onCollision(other, collType)
  if collType == ENEMY then
    self:hurt( other )
  elseif ( collType == SOLID or collType == NPC ) and ffi.C.stepBlocked( self.id ) then  -- walked into it: stopped flush
    self:sound( "bump" )
    self.blocked = true
    local pos = box( self.id ).pos
    dbglog( "hero bumped; flush at %d,%d", pos.x / self.STEP, pos.y / self.STEP )
  end
end

return hero
