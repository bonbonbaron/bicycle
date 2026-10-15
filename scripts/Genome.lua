require("bicycle")
require("io")
local Scene = require("Scene")

--------------------
---- GENES
--------------------

local Gene = {}
Gene.__index = Gene
Gene.new = function( type, val )
  local instance = {type=type, val=val}
  setmetatable( instance, Gene )
  return instance
end

function Gene.getAssetDir(geneType)
  checkType(geneType, "string")
  return "./"..gameName.."/genome/"..geneType.."/"
end

function Gene.getLuaAsset( assetType, assetName )
  checkType( assetName, "string" )
  local dir = Gene.getAssetDir( assetType )
  return require( dir..assetName )  -- returns rendered lua script
end

function Gene.getTxtAsset( assetType, assetName )
  checkType( assetType, "string" )
  checkType( assetName, "string" )
  local dir = Gene.getAssetDir( assetType )
  local fp = dir..assetName..".txt"  -- TODO make this more generalizable
  local file = io.open( fp, "r" )
  if not file then error("Couldn't open "..key.." file '"..fp.."'.") end
  local contents = file:read("*a")
  file:close()
  return contents
end

--------------------
---- SPRITES (Aseprite exports; see scripts/aseprite/sprite.lua)
--------------------

-- scripts/aseprite/sprite.lua writes one flat directory of Lua tables:
--   <sprite>.lua             { anim = <globalAnim name>, layers = { <layer>, ... } }
--   <anim>_globalAnim.lua    { strips = { <tag> = { direction, numFrames, numReps, frames = { {duration}, ... } } } }
-- and, per layer, EITHER (image layer => character)
--   <layer>_colormap.lua        { img = <ASCII frames concatenated horizontally, one space apart> }
--   <layer>_anim_src_rects.lua  { strips = { <tag> = { {x,y,w,h}, ... } } }   (x,y,w,h in pixels)
-- OR (tilemap layer => tilemap)
--   <layer>_tilemap/_tileset/_collmap/_collset.lua
-- Sprite.load() turns either into one clean table with a "kind" of "character" or "tilemap".
local Sprite = {}
Sprite.CHARACTER = "character"
Sprite.TILEMAP = "tilemap"
Sprite.ASCII_FRAME_SEP = " "     -- must match ascii.hconcat()'s separator in sprite.lua
Sprite.DEFAULT_DURATION_MS = 100
-- Aseprite's AniDir enum
Sprite.FORWARD, Sprite.REVERSE, Sprite.PING_PONG, Sprite.PING_PONG_REVERSE = 0, 1, 2, 3

-- gentable.lua turns spaces and hyphens into underscores when it names files.
function Sprite.assetName( name )
  checkType( name, "string" )
  return ( name:gsub( "[ -]", "_" ) )
end

function Sprite.getAsset( name )
  local fp = "./"..gameName.."/genome/"..Sprite.assetName( name )..".lua"
  local chunk, err = loadfile( fp )
  if not chunk then error( "Couldn't load sprite asset '"..fp.."': "..tostring(err) ) end
  return chunk()
end

local function splitLines( str )
  local lines = {}
  for line in ( str.."\n" ):gmatch( "(.-)\n" ) do
    table.insert( lines, line )
  end
  return lines
end

-- Order in which a strip's frames play for each Aseprite direction.
local function getSequence( nFrames, direction )
  local seq = {}
  if direction == Sprite.REVERSE or direction == Sprite.PING_PONG_REVERSE then
    for i = nFrames, 1, -1 do table.insert( seq, i ) end
  else
    for i = 1, nFrames do table.insert( seq, i ) end
  end
  if direction == Sprite.PING_PONG or direction == Sprite.PING_PONG_REVERSE then
    for i = nFrames - 1, 2, -1 do table.insert( seq, seq[i] ) end
  end
  return seq
end

-- An image layer: slice its ASCII strip into one string per frame and build animation strips.
function Sprite.loadCharacterLayer( layer, globAnim )
  local img = Sprite.getAsset( layer.colormap ).img
  checkType( img, "string" )
  local srcRects = layer.animSrcRects and Sprite.getAsset( layer.animSrcRects ).strips or {}
  -- Every frame's src rect starts at a distinct x; sorting them gives each frame's position in the strip.
  local xs, xToFrameIdx = {}, {}
  for _, rects in pairs( srcRects ) do
    for _, rect in ipairs( rects ) do
      if not xToFrameIdx[rect.x] then
        xToFrameIdx[rect.x] = true
        table.insert( xs, rect.x )
      end
    end
  end
  table.sort( xs )
  for i, x in ipairs( xs ) do xToFrameIdx[x] = i end
  local nFrames = math.max( #xs, 1 )
  -- Each ASCII frame is the full sprite canvas, so frames are equally wide.
  local lines = splitLines( img )
  local sepLen = #Sprite.ASCII_FRAME_SEP
  local frameW = ( #lines[1] - ( nFrames - 1 ) * sepLen ) / nFrames
  if frameW ~= math.floor( frameW ) or frameW < 1 then
    error( "Colormap '"..layer.colormap.."' is "..#lines[1].." chars wide; can't split it into "..nFrames.." frames." )
  end
  local frames = {}
  for f = 1, nFrames do
    local start = ( f - 1 ) * ( frameW + sepLen ) + 1
    local rows = {}
    for r, line in ipairs( lines ) do
      rows[r] = line:sub( start, start + frameW - 1 )
    end
    frames[f] = table.concat( rows, "\n" )
  end
  -- Strips: frame indices from the layer's src rects; timing and direction from the global anim.
  local strips = {}
  for stripName, rects in pairs( srcRects ) do
    local glob = globAnim and globAnim.strips[stripName] or {}
    local stripFrames = {}
    for i, rect in ipairs( rects ) do
      local globFrame = glob.frames and glob.frames[i]
      local durationMs = globFrame and math.floor( globFrame.duration * 1000 + 0.5 ) or Sprite.DEFAULT_DURATION_MS
      stripFrames[i] = { frameIdx = xToFrameIdx[rect.x], durationMs = durationMs }
    end
    strips[stripName] = {
      frames = stripFrames,
      sequence = getSequence( #stripFrames, glob.direction or Sprite.FORWARD ),
      numReps = glob.numReps or 0,  -- 0 means loop forever (Aseprite convention)
    }
  end
  return { kind = Sprite.CHARACTER, frames = frames, strips = strips, w = frameW, h = #lines }
end

-- A tilemap layer: tile indices laid out row-major plus its tileset's animation and collision data.
function Sprite.loadTilemapLayer( layer )
  local map = Sprite.getAsset( layer.tilemap )
  local tm = {
    kind = Sprite.TILEMAP,
    tileW = map.tileWidth,
    tileH = map.tileHeight,
    numTilesWide = map.numTilesWide,
    numTilesHigh = map.numTilesHigh,
    tiles = {},
    tileset = layer.tileset and Sprite.getAsset( layer.tileset ) or {},
    collmap = layer.collmap and Sprite.getAsset( layer.collmap ) or {},
    collset = layer.collset and Sprite.getAsset( layer.collset ) or {},
  }
  for i = 1, tm.numTilesWide * tm.numTilesHigh do
    tm.tiles[i] = map[i] or 0
  end
  return tm
end

-- Returns { name, kind, layers }. kind is "character" if the sprite's layers are images (frames, maybe
-- animated) or "tilemap" if they're tilemaps. Mixing the two in one sprite is an error: export them separately.
function Sprite.load( name )
  local spr = Sprite.getAsset( name )
  checkType( spr.layers, "table" )
  local globAnim = spr.anim and Sprite.getAsset( spr.anim.."_globalAnim" ) or nil
  local result = { name = name, layers = {} }
  for _, layer in ipairs( spr.layers ) do
    local loaded
    if layer.tilemap then
      loaded = Sprite.loadTilemapLayer( layer )
    elseif layer.colormap then
      loaded = Sprite.loadCharacterLayer( layer, globAnim )
    end  -- otherwise it's a skipped (e.g. reference) layer that only has a palette
    if loaded then
      if result.kind and result.kind ~= loaded.kind then
        error( "Sprite '"..name.."' mixes image and tilemap layers; export characters and tilemaps separately." )
      end
      result.kind = loaded.kind
      table.insert( result.layers, loaded )
    end
  end
  if not result.kind then error( "Sprite '"..name.."' has no image or tilemap layers." ) end
  return result
end

-- No ASCII tiles are exported yet (tilesets are PNGs), so draw each tile index as a glyph.
function Sprite.tilemapToAscii( tm, glyphs )
  glyphs = glyphs or " .#~^=+*%@"
  local rows = {}
  for row = 1, tm.numTilesHigh do
    local chars = {}
    for col = 1, tm.numTilesWide do
      local idx = tm.tiles[ ( row - 1 ) * tm.numTilesWide + col ]
      local g = idx % #glyphs + 1
      chars[col] = glyphs:sub( g, g )
    end
    rows[row] = table.concat( chars )
  end
  return table.concat( rows, "\n" )
end

--------------------
---- ANIMATION (driven by bridge timeouts until C++'s Animator renders src rects)
--------------------

local Animation = {}
Animation.__index = Animation

Animation.new = function( entity, character )
  local instance = { entity = entity, character = character, strip = nil, seqIdx = 1, timerId = nil, timerMs = nil }
  setmetatable( instance, Animation )
  return instance
end

function Animation:show()
  local frame = self.strip.frames[ self.strip.sequence[self.seqIdx] ]
  ffi.C.newImage( self.entity, self.character.frames[frame.frameIdx] )
  -- (Re)start the timer only when this frame's duration differs from the running timer's.
  if self.timerMs ~= frame.durationMs then
    self:stop()
    self.timerMs = frame.durationMs
    self.timerId = ffi.C.createTimer( frame.durationMs, self.entity, 0, true, ffi.C.TO_BRIDGE )
  end
end

-- Switch strips (e.g. "walk_left"). Does nothing if it's already playing or doesn't exist.
function Animation:set( stripName )
  local strip = self.character.strips[stripName]
  if not strip or strip == self.strip then return false end
  self.strip = strip
  self.seqIdx = 1
  self:show()
  return true
end

function Animation:has( stripName )
  return self.character.strips[stripName] ~= nil
end

function Animation:tick()
  if not self.strip or #self.strip.sequence < 2 then return end
  self.seqIdx = self.seqIdx % #self.strip.sequence + 1
  self:show()
end

function Animation:stop()
  if self.timerId then
    ffi.C.sys( ffi.C.STOP, ffi.C.SYS_TIMER, self.timerId )
    self.timerId, self.timerMs = nil, nil
  end
end

Gene["GeneLoader"] = {
  scene = function(entity, gene)
    checkType( entity, "number" )
    checkType( gene, "table" )
    asset = Gene.getLuaAsset( "scene", gene.val )
    Scene.new( asset )
  end,
	img = function(entity, gene)
    local imgStr = Gene.getTxtAsset("img", gene.val)  -- should use gene.type somehow too
    ffi.C.newImage( entity, imgStr )
  end,
  -- sprite = "<aseprite sprite name>": a character (static or animated) or a tilemap.
  sprite = function(entity, gene, tbl)
    local spr = Sprite.load( gene.val )
    tbl.spriteData = spr
    if spr.kind == Sprite.CHARACTER then
      local character = spr.layers[1]  -- TODO composite multiple image layers
      ffi.C.newImage( entity, character.frames[1] )
      tbl.anim = Animation.new( entity, character )
      -- Play the untagged strip, else whichever tag the genome asked for, else any.
      if not tbl.anim:set( tbl.initAnim or "DEFAULT" ) then
        tbl.anim:set( next( character.strips ) )
      end
      -- Advance frames on animation timeouts unless the genome handles timeouts itself.
      if not tbl.onTimeout then
        tbl.onTimeout = function( self, timerId, type )
          if self.anim and timerId == self.anim.timerId then self.anim:tick() end
        end
      end
    else  -- Sprite.TILEMAP
      ffi.C.newImage( entity, Sprite.tilemapToAscii( spr.layers[1] ) )
    end
  end,
  initAnim = 0,  -- do nothing; read by the sprite loader
  spriteData = 0,  -- do nothing; set by the sprite loader
  onTimeout = 0,  -- do nothing, just allow it to exist
  anim = function(entity, gene)
    --TODO insert into world anim array
  end,
  vel = function(entity, gene)
    --TODO insert into world vel array
  end,
  name = 0,  -- do nothing, just allow it to exist
  onInput = function(entity, gene) end,  -- do nothing, just allow it to exist
  onCollision = function(entity, gene) end,  -- do nothing, just allow it to exist
  onTimer = function(entity, gene) end,  -- do nothing, just allow it to exist
}

-- gameSupportedGenes is expected to be a table mapping to true.
-- Invalid keys both there and in the standard set are invalid altogether.
function Gene:load( entity, genomeName, tbl )
  checkType( entity, "number" )
  checkType( genomeName, "string" )
  if gameSupportedGenes[self.type] then
    return  -- gene is game-specific; TODO consider making this a lua loader
  end
  local loader = Gene.GeneLoader[self.type]
  if loader == 0 then return end  -- placeholder gene with nothing to load

  -- Error-handling
  if not loader then
    local recognizedGenes = ""
    for key, _ in pairs(Gene.GeneLoader) do
      recognizedGenes = recognizedGenes.."  "..key
    end
    for key, _ in pairs(gameSupportedGenes) do
      recognizedGenes = recognizedGenes.."  "..key
    end
    error( "Genome '"..genomeName.."' has unrecognized gene key '"..self.type.."'.\nThese are the available genes:\n"..recognizedGenes )
  end

  -- Time to load the gene already. Geez!!
  -- TODO load processed gene straight into the entity's table. I think that table in turn should reside in global entities.
  loader( entity, self, tbl )
end

--------------------
---- GENOMES
--------------------

-- Any genes not supported by the game engine, such as stats, items, or equipment, are presumed to be Lua scripts we can require.
local Genome = {}
Genome.__index = Genome
-- allow __newindex for blackboard functionality
Genome.new = function( name )
  local tbl = Genome.get( name ) -- tbl is the table of type-value pairs of genes (e.g. { img = "shopkeeper", ... })
  local instance = { id=ffi.C.newEntity(), tbl=tbl, genes={} }
  -- Collect genes first; loaders may add fields (e.g. anim) to the genome table.
  local genes = {}
  for type, val in pairs(instance.tbl) do
    if type ~= "sprite" then table.insert( genes, Gene.new( type, val ) ) end
  end
  -- Load the sprite first so genes like pos can rely on the image's size.
  if instance.tbl.sprite then table.insert( genes, 1, Gene.new( "sprite", instance.tbl.sprite ) ) end
  for _, gene in ipairs( genes ) do
    gene:load(instance.id, name, instance.tbl)  -- genome's self.id = entity ID
  end
  setmetatable( instance, Genome )
  return instance
end

function Genome.get(genomeName)
  checkType(genomeName, "string")
  return require("./"..gameName.."/genome/"..genomeName)
end

-- TODO call this from the loader to put a different position, dialogue, etc. on every instance of the same genome (e.g. shopkeepers w/ different items)
function Genome.inject(entity, tbl)
  -- TODO
end

Genome.Sprite = Sprite
Genome.Animation = Animation

return Genome
