-- Runs an Aseprite authoring script (test/aseprite/make_*.lua) and then the exporter (scripts/aseprite/sprite.lua)
-- without Aseprite, on an in-memory model of the small part of Aseprite's Lua API they use (Sprite, Image, Color,
-- Point, layers/frames/cels/tags, app.pixelColor). For machines without Aseprite (CI, headless boxes):
--   lua5.4 tools/aseprite/headless.lua test/aseprite/make_fairy.lua [scripts/aseprite/sprite.lua]
-- The exporter writes the same genome files it writes inside Aseprite (BICYCLE_EXPORT / BICYCLE_GENOME_PATH apply).
-- Only the .aseprite file itself isn't written (that's Aseprite's binary format): open the make_*.lua in Aseprite
-- (aseprite -b --script ...) to get it.
local makeScript, exporter = arg[1], arg[2] or "scripts/aseprite/sprite.lua"
assert( makeScript, "usage: lua5.4 tools/aseprite/headless.lua <make_*.lua> [<exporter.lua>]" )

ColorMode = { RGB = 0, GRAY = 1, INDEXED = 2, TILEMAP = 4 }
AniDir = { FORWARD = 0, REVERSE = 1, PING_PONG = 2, PING_PONG_REVERSE = 3 }

local function rgba( r, g, b, a ) return ( r & 255 ) | ( ( g & 255 ) << 8 ) | ( ( b & 255 ) << 16 ) | ( ( a & 255 ) << 24 ) end
app = { pixelColor = {
  rgba = function( r, g, b, a ) return rgba( r, g, b, a or 255 ) end,
  rgbaR = function( p ) return p & 255 end, rgbaG = function( p ) return ( p >> 8 ) & 255 end,
  rgbaB = function( p ) return ( p >> 16 ) & 255 end, rgbaA = function( p ) return ( p >> 24 ) & 255 end,
  grayaV = function( p ) return p & 255 end, grayaA = function( p ) return ( p >> 8 ) & 255 end,
  tileI = function( p ) return p & 0x1fffffff end,
}, alert = function( m ) print( "alert: " .. tostring( type( m ) == "table" and m.text or m ) ) end,
  fs = { filePath = function( p ) return p:match( "^(.*)[/\\]" ) or "" end,
         fileTitle = function( p ) return p:match( "([^/\\]*)%.[^.]*$" ) or p end,
         joinPath = function( a, b ) return a .. "/" .. b end } }

function Color( t )
  local c = { red = t.r or t.red or 0, green = t.g or t.green or 0, blue = t.b or t.blue or 0, alpha = t.a or t.alpha or 255 }
  c.rgbaPixel = rgba( c.red, c.green, c.blue, c.alpha )
  return c
end
function Point( x, y ) return { x = x, y = y } end

local Img = {}
Img.__index = Img
function Image( w, h, mode )
  local px = {}
  for i = 1, w * h do px[i] = 0 end
  return setmetatable( { width = w, height = h, colorMode = mode or ColorMode.RGB, _px = px }, Img )
end
function Img:drawPixel( x, y, c )
  if x < 0 or y < 0 or x >= self.width or y >= self.height then return end
  self._px[y * self.width + x + 1] = type( c ) == "table" and c.rgbaPixel or c
end
function Img:getPixel( x, y ) return self._px[y * self.width + x + 1] end
function Img:pixels()
  local i = 0
  return function()
    if i >= self.width * self.height then return nil end
    local v, x, y = self._px[i + 1], i % self.width, i // self.width
    i = i + 1
    return setmetatable( { x = x, y = y }, { __call = function() return v end } )
  end
end
function Img:saveAs( path ) print( "(headless: not writing image " .. path .. ")" ) end

local Spr = {}
Spr.__index = Spr
function Sprite( w, h, mode )
  local s = setmetatable( { width = w, height = h, colorMode = mode or ColorMode.RGB, frames = {}, tags = {}, layers = {},
    properties = {}, filename = "Sprite.aseprite",
    palettes = { { getColor = function( _, i ) return Color{ r = i, g = i, b = i } end } } }, Spr )
  s:newEmptyFrame()
  s:newLayer().name = "Layer 1"
  app.sprite = s
  return s
end
function Spr:newEmptyFrame()
  local f = { frameNumber = #self.frames + 1, duration = 0.1 }
  if #self.frames > 0 then self.frames[#self.frames].next = f end
  self.frames[#self.frames + 1] = f
  return f
end
function Spr:newLayer()
  local l = { name = "Layer " .. ( #self.layers + 1 ), isImage = true, isGroup = false, isTilemap = false, isReference = false, _cels = {} }
  function l:cel( f ) return self._cels[type( f ) == "table" and f.frameNumber or f] end
  self.layers[#self.layers + 1] = l
  return l
end
function Spr:newCel( layer, frame, img, pos )
  pos = pos or Point( 0, 0 )
  local c = { image = img, position = pos, bounds = { x = pos.x, y = pos.y, w = img.width, h = img.height, width = img.width, height = img.height } }
  layer._cels[type( frame ) == "table" and frame.frameNumber or frame] = c
  return c
end
function Spr:newTag( from, to )
  local t = { name = "Tag", fromFrame = self.frames[from], toFrame = self.frames[to], frames = to - from + 1, aniDir = AniDir.FORWARD, repeats = 0 }
  self.tags[#self.tags + 1] = t
  return t
end
function Spr:saveAs( path )
  self.filename = path
  print( "(headless: not writing " .. path .. "; run the script in Aseprite for the .aseprite file)" )
end

dofile( makeScript )
assert( app.sprite, makeScript .. " made no sprite" )
dofile( exporter )
