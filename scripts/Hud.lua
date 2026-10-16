require("bicycle")

-- Meters glued to the top of the screen, e.g. a health bar:
--   local hp = Hud.bar( "HP", 10 )   -- starts full
--   hp:set( 7 )
-- Each render mode draws it its own way: ASCII as text (HP [#######---] 7/10), 2D and 3D as a graphical bar
-- colored by how full it is. hp:sprites( "genome/ui/hp_frame.bimg", "genome/ui/hp_fill.bimg" ) draws it from
-- images in the pixel modes instead (the fill is cropped to the fraction).
-- Bars stack down from the top-left inside the scene's border unless you pass x and y (screen cells).
local Hud = {}
local Bar = {}
Bar.__index = Bar
local nextRow = 1
local bars = {}  -- every bar, so a scene change can put them back (the HUD is drawn by the scene)

function Hud.bar( label, max, len, x, y )
  len = len or max
  if not y then
    y = nextRow
    nextRow = nextRow + 1
  end
  local id = ffi.C.addHudBar( label or "", max, len, x or 2, y )
  if id < 0 then error( "Hud.bar needs a scene to draw on; make one first." ) end
  local bar = setmetatable( { id = id, max = max, value = max, label = label or "", len = len, x = x or 2, y = y }, Bar )
  table.insert( bars, bar )
  return bar
end

-- Scene.goto: the new scene gets the same bars, with their values and sprites. A bar that shouldn't follow the
-- player into the next scene (e.g. a boss's) is dropped with Hud.forget( bar ).
function Hud.rebuild()
  for _, b in ipairs( bars ) do
    b.id = ffi.C.addHudBar( b.label, b.max, b.len, b.x, b.y )
    ffi.C.setHudBar( b.id, b.value )
    if b.frame then ffi.C.setHudBarSprites( b.id, b.frame, b.fill ) end
  end
end

function Hud.forget( bar )
  for i, b in ipairs( bars ) do
    if b == bar then table.remove( bars, i ) return end
  end
end

function Bar:set( value )
  self.value = math.max( 0, math.min( self.max, value ) )
  ffi.C.setHudBar( self.id, self.value )
  return self.value
end

function Bar:sprites( framePath, fillPath )
  self.frame, self.fill = framePath, fillPath
  return ffi.C.setHudBarSprites( self.id, framePath, fillPath )
end

function Bar:add( delta )
  return self:set( self.value + delta )
end

return Hud
