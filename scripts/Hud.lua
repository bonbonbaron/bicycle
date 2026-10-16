require("bicycle")

-- Meters glued to the top of the screen, e.g. a health bar:
--   local hp = Hud.bar( "HP", 10 )   -- starts full; drawn as  HP [##########] 10/10
--   hp:set( 7 )                      -- HP [#######---] 7/10
-- Bars stack down from the top-left inside the scene's border unless you pass x and y (screen cells).
local Hud = {}
local Bar = {}
Bar.__index = Bar
local nextRow = 1

function Hud.bar( label, max, len, x, y )
  len = len or max
  if not y then
    y = nextRow
    nextRow = nextRow + 1
  end
  local id = ffi.C.addHudBar( label or "", max, len, x or 2, y )
  if id < 0 then error( "Hud.bar needs a scene to draw on; make one first." ) end
  return setmetatable( { id = id, max = max, value = max }, Bar )
end

function Bar:set( value )
  self.value = math.max( 0, math.min( self.max, value ) )
  ffi.C.setHudBar( self.id, self.value )
  return self.value
end

function Bar:add( delta )
  return self:set( self.value + delta )
end

return Hud
