require('bicycle')
local Sound = require('Sound')
local Hud = require('Hud')
require('Dialogue')
-- The player: an animated Aseprite character (see scripts/aseprite/sprite.lua) who walks with WASD,
-- is blocked by solid things, gets hurt by enemies (health bar up top), and talks to NPCs with E or Space.
local hero = { sprite="pilgrim", initAnim="walk_down", coll={ type="hero" } }

local STEP = 256  -- one cell in fixed-point (FIXEDPT_DEC_BITS = 8)
local DIRS = {
  [ffi.C.A] = { strip="walk_left",  dx=-2 * STEP, dy=0 },  -- 2 columns ~ 1 row, since cells are tall
  [ffi.C.D] = { strip="walk_right", dx= 2 * STEP, dy=0 },
  [ffi.C.W] = { strip="walk_up",    dx=0, dy=-STEP },
  [ffi.C.S] = { strip="walk_down",  dx=0, dy= STEP },
}
local SOLID, ENEMY, NPC = 2, 4, 5  -- Genome.COLL
local BLOCKERS = { [SOLID]=true, [ENEMY]=true, [NPC]=true }
local MAX_HEALTH = 10
local DAMAGE = 2
local KNOCKBACK = 6 * STEP      -- columns pushed away from whatever hurt us
local TICK_MS = 100             -- flash/invulnerability timer period
local INVULNERABLE_TICKS = 10   -- ~1 s of invulnerability (and flashing) after a hit
local TALK_REACH = 2            -- cells of gap allowed between us and someone we talk to
local TICK = 1                  -- timeout type of our timer

local function box( id ) return ffi.C.World_getBoxes().arr[id] end

function hero:onSpawn()
  local pos = box( self.id ).pos
  self.spawn = { x = pos.x, y = pos.y }
  self.health = MAX_HEALTH
  self.hp = Hud.bar( "HP", MAX_HEALTH )
  self.invulnerable = 0
  ffi.C.createTimer( TICK_MS, self.id, TICK, true, ffi.C.TO_BRIDGE )
end

-- While touching something solid (e.g. it walked into us), refuse moves toward its center.
function hero:blockedBy(dir)
  local me = box( self.id )
  for other, _ in pairs( self.touching or {} ) do
    local b = box( other )
    local dx = ( b.pos.x + b.size.w / 2 ) - ( me.pos.x + me.size.w / 2 )
    local dy = ( b.pos.y + b.size.h / 2 ) - ( me.pos.y + me.size.h / 2 )
    if dir.dx * dx + dir.dy * dy * 4 > 0 then return true end  -- rows are ~2x as tall as columns are wide
  end
  return false
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

function hero:onInput(input)
  input = tonumber( input )
  if input == ffi.C.Q then
    ffi.C.popWindow()
    return
  end
  -- While a Dialogue is up it has the input focus, so we don't move (or get here at all).
  if input == ffi.C.E or input == ffi.C.Space then
    local npc = self:speakerInReach()
    if npc then
      Sound.play( "blip" )
      dbglog( "talk to %s", npc.name )
      Dialogue.new( npc.name..":\n"..table.concat( npc.lines, "\n\n" ), nil, nil, nil, nil, function() dbglog( "talk done" ) end )
    end
    return
  end
  local dir = DIRS[input]
  if dir and self:blockedBy( dir ) then
    self.anim:set( dir.strip )
    Sound.play( "bump" )
    dbglog( "hero blocked" )
  elseif dir then
    self.anim:set( dir.strip )  -- no-op if already walking this way
    local pos = box( self.id ).pos
    self.prevPos = { x = pos.x, y = pos.y }  -- where to step back to if this move bumps into something solid
    pos.x = pos.x + dir.dx
    pos.y = pos.y + dir.dy
    dbglog( "hero move to %d,%d", pos.x / STEP, pos.y / STEP )
  end
end

function hero:hurt( by )
  if self.invulnerable > 0 or self.health <= 0 then
    dbglog( "hero hit while invulnerable; no damage" )
    return
  end
  self.health = self.hp:set( self.health - DAMAGE )
  self.invulnerable = INVULNERABLE_TICKS
  Sound.play( "hurt" )
  -- Knock back, away from whatever hit us.
  local me, b = box( self.id ), box( by )
  local away = ( me.pos.x + me.size.w / 2 ) < ( b.pos.x + b.size.w / 2 ) and -1 or 1
  me.pos.x = me.pos.x + away * KNOCKBACK
  dbglog( "hero hurt: health %d", self.health )
  if self.health <= 0 then
    dbglog( "game over" )
    Dialogue.new( "You collapse in the tall grass...\n\n...and wake at the edge of the meadow, rested. Press E to try again.",
      nil, nil, nil, nil, function() self:respawn() end )
  end
end

function hero:respawn()
  local pos = box( self.id ).pos
  pos.x, pos.y = self.spawn.x, self.spawn.y
  self.health = self.hp:set( MAX_HEALTH )
  self.invulnerable = INVULNERABLE_TICKS
  self.touching = {}
  dbglog( "respawn: health %d", self.health )
end

-- Flash while invulnerable.
function hero:onTimeout(timerId, timeoutType)
  if self.invulnerable > 0 then
    self.invulnerable = self.invulnerable - 1
    ffi.C.setHidden( self.id, self.invulnerable % 2 == 1 )
  end
end

function hero:onCollision(other, collType)
  if BLOCKERS[collType] then
    self.touching = self.touching or {}
    self.touching[other] = true
    if self.prevPos then  -- we walked into it: step back
      local pos = box( self.id ).pos
      pos.x, pos.y = self.prevPos.x, self.prevPos.y
      self.prevPos = nil
      if collType ~= ENEMY then Sound.play( "bump" ) end
      dbglog( "hero bumped; back to %d,%d", pos.x / STEP, pos.y / STEP )
    end
  end
  if collType == ENEMY then
    self:hurt( other )
  end
end

function hero:onUncollision(other, collType)
  if self.touching then self.touching[other] = nil end
end

return hero
