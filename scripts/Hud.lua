require("bicycle")

-- The HUD: meters and text labels in the scene's HUD layer, glued to the screen over the world (src/lua/LuaHud.cpp).
--   local hp = Hud.bar( "HP", 10 )                -- a meter, starting full; hp:set( 7 ), hp:add( -1 )
--   local label = Hud.text( "", "right", "top" )  -- a label; label:set( string.format( "%.0f fps", fps() ) )
-- Bars stack down from the top-left inside the scene's border unless you pass x and y (screen cells).
-- Hud.text( text, ax, ay ): ax = "left" | "right" | "center" | a cell x, ay = "top" | "bottom" | a cell y (defaults
-- "left", "top"); "right" keeps the text's right edge at the screen's right edge as it changes.
-- ASCII draws them as text (HP [#######---] 7/10); 2D and 3D as a graphical bar colored by how full it is, and
-- text on a plate. hp:sprites( "genome/ui/hp_frame.bimg", "genome/ui/hp_fill.bimg" ) draws a meter from images in
-- the pixel modes instead (the fill is cropped to the fraction).
-- Everything here follows the player into the next scene (Hud.rebuild, from Scene.goto) until Hud.forget( item ).
local Hud = {}
local items = {}  -- in the order made; each knows how to put itself on a scene's HUD (:place)
local nextRow = 1

local Bar = {}
Bar.__index = Bar

function Bar:place()
  self.id = ffi.C.addHudBar( self.label, self.max, self.len, self.x, self.y )
  if self.id < 0 then error( "Hud.bar needs a scene to draw on; make one first." ) end
  ffi.C.setHudBar( self.id, self.value )
  if self.frame or self.fill then ffi.C.setHudBarSprites( self.id, self.frame, self.fill ) end
end

function Bar:set( value )
  self.value = math.max( 0, math.min( self.max, value ) )
  ffi.C.setHudBar( self.id, self.value )
  return self.value
end

function Bar:add( delta )
  return self:set( self.value + delta )
end

function Bar:sprites( framePath, fillPath )
  self.frame, self.fill = framePath, fillPath
  return ffi.C.setHudBarSprites( self.id, framePath, fillPath )
end

local Text = {}
Text.__index = Text

function Text:place()
  self.id = ffi.C.addHudText( self.text, self.x, self.y )
  if self.id < 0 then error( "Hud.text needs a scene to draw on; make one first." ) end
end

function Text:set( text )
  self.text = tostring( text or "" )
  ffi.C.setHudText( self.id, self.text )
  return self.text
end

local function make( item )
  item:place()
  table.insert( items, item )
  return item
end

function Hud.bar( label, max, len, x, y )
  if not y then
    y = nextRow
    nextRow = nextRow + 1
  end
  return make( setmetatable( { label = label or "", max = max, value = max, len = len or max, x = x or 2, y = y }, Bar ) )
end

local AX = { left = -1, right = -2, center = -3 }  -- gfx::Widget::Edge
local AY = { top = -1, bottom = -2 }

function Hud.text( text, ax, ay )
  local x = type( ax ) == "number" and ax or AX[ax or "left"]
  local y = type( ay ) == "number" and ay or AY[ay or "top"]
  if not x then error( "Hud.text: ax is \"left\", \"right\", \"center\" or a cell x, not "..tostring( ax ) ) end
  if not y then error( "Hud.text: ay is \"top\", \"bottom\" or a cell y, not "..tostring( ay ) ) end
  return make( setmetatable( { text = tostring( text or "" ), x = x, y = y }, Text ) )
end

-- Scene.goto: the new scene gets the same HUD, with its values, texts and sprites.
function Hud.rebuild()
  for _, item in ipairs( items ) do item:place() end
end

-- Takes an item off the HUD, now and for later scenes (e.g. a boss's bar).
function Hud.forget( item )
  for i, it in ipairs( items ) do
    if it == item then
      ffi.C.removeHudItem( it.id )
      table.remove( items, i )
      return
    end
  end
end

return Hud
