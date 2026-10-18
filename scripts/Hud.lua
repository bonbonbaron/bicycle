require("bicycle")

-- Meters glued to the top of the screen, e.g. a health bar:
--   local hp = Hud.bar( "HP", 10 )   -- starts full
--   hp:set( 7 )
-- Each render mode draws it its own way: ASCII as text (HP [#######---] 7/10), 2D and 3D as a graphical bar
-- colored by how full it is. hp:sprites( "genome/ui/hp_frame.bimg", "genome/ui/hp_fill.bimg" ) draws it from
-- images in the pixel modes instead (the fill is cropped to the fraction).
-- Bars stack down from the top-left inside the scene's border unless you pass x and y (screen cells).
--
-- Text labels: Hud.text( text, ax, ay ), ax = "left" | "right" | "center" | a cell x, ay = "top" | "bottom" | a
-- cell y (defaults "left", "top"). "right" keeps the text's right edge at the screen's right edge as it changes.
--   local label = Hud.text( "", "right", "top" )
--   label:set( string.format( "%.0f fps", fps() ) )   -- e.g. from onTimer
-- ASCII prints it as plain text; 2D and 3D draw it on a plate like the bars'.
local Hud = {}
local Bar = {}
Bar.__index = Bar
local nextRow = 1
local bars = {}  -- every bar, so a scene change can put them back (the HUD is drawn by the scene)
local Text = {}
Text.__index = Text
local texts = {}  -- every text label, likewise
local AX = { left = -1, right = -2, center = -3 }
local AY = { top = -1, bottom = -2 }

function Hud.text( text, ax, ay )
  local x = type( ax ) == "number" and ax or AX[ax or "left"]
  local y = type( ay ) == "number" and ay or AY[ay or "top"]
  if not x then error( "Hud.text: ax is \"left\", \"right\", \"center\" or a cell x, not "..tostring( ax ) ) end
  if not y then error( "Hud.text: ay is \"top\", \"bottom\" or a cell y, not "..tostring( ay ) ) end
  local t = setmetatable( { text = tostring( text or "" ), x = x, y = y }, Text )
  t.id = ffi.C.addHudText( t.text, x, y )
  if t.id < 0 then error( "Hud.text needs a scene to draw on; make one first." ) end
  table.insert( texts, t )
  return t
end

function Text:set( text )
  self.text = tostring( text or "" )
  ffi.C.setHudText( self.id, self.text )
  return self.text
end

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
  for _, t in ipairs( texts ) do t.id = ffi.C.addHudText( t.text, t.x, t.y ) end
end

-- A text label forgotten is also blanked in the current scene.
function Hud.forget( item )
  for i, b in ipairs( bars ) do
    if b == item then table.remove( bars, i ) return end
  end
  for i, t in ipairs( texts ) do
    if t == item then ffi.C.setHudText( t.id, "" ) table.remove( texts, i ) return end
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
