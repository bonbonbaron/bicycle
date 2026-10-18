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

local Ids = require("Ids")  -- names -> build-time ids (README, "IDs")
local Layout = require("GenomeLayout")  -- where each kind of genome file lives (README, "Genome layout")

function Gene.getAssetDir(geneType)
  checkType(geneType, "string")
  return "./"..gameName.."/genome/"..Layout.geneDir( geneType )
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
  local contents = Asset.read( fp )
  if not contents then error("Couldn't open "..assetType.." file '"..fp.."'.") end
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
  local n = Sprite.assetName( name )
  local fp = "./"..gameName.."/genome/"..Layout.dirFor( n )..n..".lua"
  local result, err = Asset.dofile( fp )
  if err then error( "Couldn't load sprite asset '"..fp.."': "..tostring(err) ) end
  return result
end

-- Path of a pixel export's .bimg (see scripts/aseprite/sprite.lua).
function Sprite.pixelPath( name )
  local n = Sprite.assetName( name )
  return "./"..gameName.."/genome/"..Layout.dirFor( n )..n..".bimg"
end

-- Render mode of this build: "ascii", "2d" or "3d" ("ascii" outside the engine, e.g. headless tests).
-- Pixel modes use a sprite's pixel export (.bimg) when it has one and fall back to its ASCII art otherwise.
Sprite.MODES = { [0] = "ascii", [1] = "2d", [2] = "3d" }
function Sprite.renderMode()
  local ok, mode = pcall( function() return ffi.C.renderMode() end )
  return ok and Sprite.MODES[mode] or "ascii"
end
function Sprite.usePixels( hasPixels )
  return hasPixels and Sprite.renderMode() ~= "ascii"
end

function Sprite.exists( name )
  local n = Sprite.assetName( name )
  return Asset.exists( "./"..gameName.."/genome/"..Layout.dirFor( n )..n..".lua" )
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
  if Sprite.usePixels( layer.pixels ) or not layer.colormap then
    return Sprite.loadPixelLayer( layer, globAnim )
  end
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

-- Strips from a layer's src rects: frame indices (by each rect's x) plus timing and direction from the global anim.
local function loadStrips( srcRects, globAnim, xToFrameIdx )
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
      numReps = glob.numReps or 0,
    }
  end
  return strips
end

-- An image layer's pixel export: a .bimg frame strip (frame f at x = (f - 1) * canvas width, in pixels).
-- Sizes are in cells like everything else: a pixel is 2 cells wide and 1 tall, as in the ASCII export.
function Sprite.loadPixelLayer( layer, globAnim )
  if not layer.pixels then error( "Layer '"..tostring( layer.colormap ).."' has no pixel export; re-export it with BICYCLE_EXPORT=pixels or both." ) end
  local srcRects = layer.animSrcRects and Sprite.getAsset( layer.animSrcRects ).strips or {}
  local xToFrameIdx, canvasW, canvasH = {}, nil, nil
  for _, rects in pairs( srcRects ) do
    for _, rect in ipairs( rects ) do
      xToFrameIdx[rect.x] = math.floor( rect.x / rect.w ) + 1
      canvasW, canvasH = rect.w, rect.h
    end
  end
  canvasW = canvasW or layer.pixelW
  canvasH = canvasH or layer.pixelH
  local nFrames = math.max( math.floor( ( layer.pixelW or canvasW ) / canvasW ), 1 )
  return { kind = Sprite.CHARACTER, pixels = { layer.pixels }, nFrames = nFrames,
           strips = loadStrips( srcRects, globAnim, xToFrameIdx ), w = 2 * canvasW, h = canvasH }
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
    bg = Sprite.bgSettings( layer.bg ),
  }
  for i = 1, tm.numTilesWide * tm.numTilesHigh do
    tm.tiles[i] = map[i] or 0
  end
  return tm
end

-- Background modes, matching C++'s LayerType (src/v/Scene.h). Set per tilemap layer in Aseprite via its
-- user data, e.g. "mode=parallax parallax=50" (see scripts/aseprite/README.txt).
Sprite.BG_MODES = { fixed = 0, glued = 1, parallax = 2, loop = 3, autoloop = 4 }
Sprite.FRAMES_PER_SECOND = 30  -- Constants::FRAMES_PER_SECOND

-- Fills in defaults for exports without settings (or older ones).
function Sprite.bgSettings( bg )
  bg = bg or {}
  local mode = bg.mode or "fixed"
  if not Sprite.BG_MODES[mode] then error( "Unknown background mode '"..tostring( mode ).."'." ) end
  local parallax = bg.parallax or ( mode == "parallax" and 50 or 100 )
  return {
    mode = mode,
    parallax = parallax,
    parallaxy = bg.parallaxy or parallax,
    vx = bg.vx or ( mode == "autoloop" and not bg.vy and 4 or 0 ),
    vy = bg.vy or 0,
    x = bg.x or 0,
    y = bg.y or 0,
  }
end

-- Pushes one tilemap layer into the current scene as a background layer in its mode.
function Sprite.addBgLayer( tm )
  local bg = tm.bg
  local FIXEDPT = 256  -- 1 << FIXEDPT_DEC_BITS
  if Sprite.usePixels( tm.tileset and tm.tileset.pixels ) then
    local n = tm.numTilesWide * tm.numTilesHigh
    local tiles = ffi.new( "int[?]", n )
    for i = 1, n do tiles[i - 1] = tm.tiles[i] end
    local ok = ffi.C.addBgLayerPixels( Sprite.pixelPath( tm.tileset.pixels ), tiles, tm.numTilesWide, tm.numTilesHigh,
      tm.tileW, tm.tileH, Sprite.BG_MODES[bg.mode], bg.parallax, bg.parallaxy,
      math.floor( bg.vx * FIXEDPT / Sprite.FRAMES_PER_SECOND + 0.5 ),
      math.floor( bg.vy * FIXEDPT / Sprite.FRAMES_PER_SECOND + 0.5 ),
      bg.x, bg.y )
    if not ok then error( "Couldn't load tileset pixels '"..tm.tileset.pixels.."'." ) end
    return
  end
  ffi.C.addBgLayerEx( Sprite.tilemapToAscii( tm ), Sprite.BG_MODES[bg.mode], bg.parallax, bg.parallaxy,
    math.floor( bg.vx * FIXEDPT / Sprite.FRAMES_PER_SECOND + 0.5 ),  -- cells/second -> fixed-point cells/frame
    math.floor( bg.vy * FIXEDPT / Sprite.FRAMES_PER_SECOND + 0.5 ),
    bg.x, bg.y )
end

-- Overlays characters' frames in order (first = bottom, like Aseprite's layer order). Spaces are transparent.
-- The exporter gives every layer one full-canvas frame per sprite frame, so frames line up by index.
function Sprite.composite( characters )
  local base = characters[1]
  if #characters == 1 then return base end
  if base.pixels then  -- pixel layers composite in C++ (newPixelImage): just list them, bottom first
    local pixels = {}
    for _, c in ipairs( characters ) do
      if not c.pixels then error( "Can't composite pixel and ASCII layers of one sprite." ) end
      table.insert( pixels, c.pixels[1] )
    end
    return { kind = Sprite.CHARACTER, pixels = pixels, nFrames = base.nFrames, strips = base.strips, w = base.w, h = base.h }
  end
  local frames = {}
  for f = 1, #base.frames do
    local rows = splitLines( base.frames[f] )
    for c = 2, #characters do
      local upper = characters[c].frames[f]
      if not upper then error( "Image layers have different frame counts; can't composite frame "..f.."." ) end
      for r, line in ipairs( splitLines( upper ) ) do
        local below = rows[r] or ""
        local out = {}
        for i = 1, math.max( #below, #line ) do
          local ch = line:sub( i, i )
          if ch == "" or ch == " " then ch = below:sub( i, i ) end
          out[i] = ch ~= "" and ch or " "
        end
        rows[r] = table.concat( out )
      end
    end
    frames[f] = table.concat( rows, "\n" )
  end
  return { kind = Sprite.CHARACTER, frames = frames, strips = base.strips, w = base.w, h = base.h }
end

-- Returns { name, kind, layers }. kind is "character" if the sprite's layers are images (frames, maybe
-- animated) or "tilemap" if they're tilemaps. Mixing the two in one sprite is an error: export them separately.
-- Characters also get result.character: all image layers composited into one.
-- Loaded once per sprite name and shared by every body using it, palette variants included: the graybody
-- (pixels, animation strips/rects, colormap) is palette-agnostic, so a red and a green beetle use one table.
Sprite.cache = {}

function Sprite.load( name )
  local key = name.."@"..Sprite.renderMode()
  if Sprite.cache[key] then return Sprite.cache[key] end
  local result = Sprite.loadUncached( name )
  Sprite.cache[key] = result
  return result
end

-- A color palette variant (genome palette = "<name>_palette", in body/palette/color/) as the engine's
-- "index:rgba,..." string. The file is either { colors = { [index] = rgba, ... } } (hand-written swaps; rgba is
-- 0xAABBGGRR, R in the low byte like .bimg) or an exporter palette set { members = { [rgba] = index } }.
function Sprite.paletteSpec( name )
  local pal = Sprite.getAsset( name )
  local items = {}
  if pal.colors then
    for i, c in pairs( pal.colors ) do items[#items + 1] = i..":"..c end
  else
    for c, i in pairs( pal.members or {} ) do items[#items + 1] = i..":"..c end
  end
  table.sort( items )
  return table.concat( items, "," )
end

-- A collision palette (genome collpal = "<name>_collset", in body/palette/collision/): { types = { [region] =
-- "<Genome.COLL name>" } }, mapping the graybody's collision regions (1 = the whole body) to collision types.
function Sprite.collisionType( collpal, region )
  local cp = Sprite.getAsset( collpal )
  local t = cp.types and cp.types[region or 1]
  if t == nil then error( "Collision palette '"..collpal.."' has no type for region "..tostring( region or 1 ).."." ) end
  return t
end

function Sprite.loadUncached( name )
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
  if result.kind == Sprite.CHARACTER then
    result.character = Sprite.composite( result.layers )
  end
  return result
end

-- Draws a tilemap from its tileset's ASCII tiles (tileset.ascii[tileIdx + 1], exported by sprite.lua).
-- Older exports without ASCII tiles fall back to one glyph per tile.
function Sprite.tilemapToAscii( tm, glyphs )
  local asciiTiles = tm.tileset and tm.tileset.ascii
  local rows = {}
  if asciiTiles and next( asciiTiles ) then
    local tileLines, blank = {}, nil
    for i, str in pairs( asciiTiles ) do tileLines[i] = splitLines( str ) end
    for row = 1, tm.numTilesHigh do
      for tileRow = 1, tm.tileH do
        local chars = {}
        for col = 1, tm.numTilesWide do
          local lines = tileLines[ tm.tiles[ ( row - 1 ) * tm.numTilesWide + col ] + 1 ]
          blank = blank or string.rep( " ", tm.tileW * 2 )
          chars[col] = lines and lines[tileRow] or blank
        end
        table.insert( rows, table.concat( chars ) )
      end
    end
  else
    glyphs = glyphs or " .#~^=+*%@"
    for row = 1, tm.numTilesHigh do
      local chars = {}
      for col = 1, tm.numTilesWide do
        local idx = tm.tiles[ ( row - 1 ) * tm.numTilesWide + col ]
        local g = idx % #glyphs + 1
        chars[col] = glyphs:sub( g, g )
      end
      rows[row] = table.concat( chars )
    end
  end
  return table.concat( rows, "\n" )
end

-- Lays a character's frames side by side (one space apart) as one image, and gives each frame's src rect
-- in character cells. C++'s Animator points the entity at one rect at a time; Camera draws only that part.
function Sprite.toStrip( character )
  local lines = {}
  local rects = {}
  for f, frame in ipairs( character.frames ) do
    for r, line in ipairs( splitLines( frame ) ) do
      lines[r] = lines[r] and ( lines[r]..Sprite.ASCII_FRAME_SEP..line ) or line
    end
    rects[f] = { x = ( f - 1 ) * ( character.w + #Sprite.ASCII_FRAME_SEP ), y = 0, w = character.w, h = character.h }
  end
  return table.concat( lines, "\n" ), rects
end

--------------------
---- ANIMATION (a thin handle on C++'s Animator)
--------------------

local Animation = {}
Animation.__index = Animation

-- Hands the character's strips to C++: frames in play order (directions already expanded), each with its
-- src rect and duration. Expects the entity's image to be Sprite.toStrip()'s image.
Animation.new = function( entity, character, rects )
  for stripName, strip in pairs( character.strips ) do
    local id = Ids.define( "anim", stripName )  -- tags come from the art: resolved once, here
    ffi.C.newAnimStrip( entity, id, strip.numReps == 0 )
    for _, seqIdx in ipairs( strip.sequence ) do
      local frame = strip.frames[seqIdx]
      local rect = rects[frame.frameIdx]
      ffi.C.addAnimFrame( entity, id, frame.durationMs, rect.x, rect.y, rect.w, rect.h )
    end
  end
  local instance = { entity = entity, character = character }
  setmetatable( instance, Animation )
  return instance
end

-- Switch strips (e.g. "walk_left"). Returns false if it's already playing or doesn't exist.
-- In 3D a rigged model plays the action named by the strip's tag: the part before the first "_" ("walk_left" ->
-- "walk", "idle" -> "idle"); the renderer shows "idle" when a walking entity stands still.
Animation.rigged = {}  -- entity -> true for entities drawn with a model
function Animation:set( stripName )
  if Animation.rigged[self.entity] and type( stripName ) == "string" then
    ffi.C.setModelAction( self.entity, stripName:match( "^[^_]+" ) or stripName )
  end
  local id = Ids.get( "anim", stripName )  -- not an animation tag anywhere: a clear error (a typo)
  return self:has( stripName ) and ffi.C.setAnim( self.entity, id )
end

function Animation:has( stripName )
  return self.character.strips[stripName] ~= nil
end

function Animation:stop()
  ffi.C.stopAnim( self.entity )
end

function Animation:frameIdx()
  return ffi.C.getAnimFrameIdx( self.entity )
end

-- Collision types games can refer to by name in coll genes; onCollision( other, type ) receives the number.
-- The generator (tools/ids/genids.lua) reads this table: the ids in gen/ids.lua/ids.h (ids::Coll) are these values.
local COLL = { hero = 1, solid = 2, coin = 3, enemy = 4, npc = 5, door = 6, fire = 7 }

-- The timer gene: timer = <ms> | { ms, rep = true, type = 0 }.
local function startTimer( entity, t, tbl )
  if type( t ) == "number" then t = { ms = t } end
  checkType( t, "table" )
  local id = ffi.C.createTimer( t.ms or 1000, entity, t.type or 0, t.rep ~= false, ffi.C.TO_BRIDGE )
  if tbl then tbl.timerId = id end
  return id
end

Gene["GeneLoader"] = {
  scene = function(entity, gene)
    checkType( entity, "number" )
    checkType( gene, "table" )
    local asset = Gene.getLuaAsset( "scene", gene.val )
    asset.file = gene.val
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
      local character = spr.character  -- all image layers composited
      local rects
      if character.pixels then
        local paths = {}
        for i, name in ipairs( character.pixels ) do paths[i] = Sprite.pixelPath( name ) end
        local pal = tbl.palette and Sprite.paletteSpec( tbl.palette ) or ""
        if not ffi.C.newPixelImagePal( entity, table.concat( paths, "\n" ), pal ) then
          error( "Couldn't load sprite pixels for '"..gene.val.."'." )
        end
        rects = {}  -- frames side by side, no gap, each the canvas size (in cells)
        for f = 1, character.nFrames do
          rects[f] = { x = ( f - 1 ) * character.w, y = 0, w = character.w, h = character.h }
        end
      else
        local img
        img, rects = Sprite.toStrip( character )
        ffi.C.newImage( entity, img )
      end
      tbl.anim = Animation.new( entity, character, rects )
      -- Play the untagged strip, else whichever tag the genome asked for, else any.
      if not tbl.anim:set( tbl.initAnim or "DEFAULT" ) then
        tbl.anim:set( next( character.strips ) )
      end
    else  -- Sprite.TILEMAP: every layer becomes a background layer of the current scene, back to front
      for _, tm in ipairs( spr.layers ) do
        Sprite.addBgLayer( tm )
      end
    end
  end,
  -- coll = { type = "solid", layer = 0, box = { x, y, w, h } }: collide with entities on the same layer.
  -- type is a name from Genome.COLL (or a number); box is relative to the entity, in cells (default: whole entity).
  coll = function(entity, gene, tbl)
    local c = gene.val
    checkType( c, "table" )
    -- Without a type, a genome's collision palette (collpal) picks it for the body's region (default 1).
    local name = c.type or ( tbl and tbl.collpal and Sprite.collisionType( tbl.collpal, c.region ) ) or "solid"
    local collType = type( name ) == "number" and name or COLL[name]
    if not collType then error( "Unknown collision type '"..tostring( c.type ).."'." ) end
    local box = c.box or { 0, 0, 0, 0 }
    ffi.C.addCollider( entity, c.layer or 0, collType, box[1], box[2], box[3], box[4] )
  end,
  initAnim = 0,  -- do nothing; read by the sprite loader
  palette = 0,   -- color palette variant: read by the sprite loader (Sprite.paletteSpec)
  collpal = 0,   -- collision palette: read by the coll loader (Sprite.collisionType)
  -- model = "<Blender object>": in 3D mode the entity is drawn as genome/body/model/<name> (exported by
  -- tools/blender; packed as .bmesh) instead of its sprite; other modes ignore it.
  model = function(entity, gene)
    if Sprite.renderMode() ~= "3d" then return end
    if not ffi.C.setModel( entity, Ids.asset( "model", gene.val ) ) then
      error( "Couldn't load model '"..gene.val.."' (meshes come from the asset pack: build the 'assets' target)." )
    end
    Animation.rigged[entity] = true
  end,
  motion = 0,  -- started by Scene once every entity is placed (Genome.startMotion), so targets can be named
  -- persist = true: the entity survives scene changes (the hero, a companion); see Scene.goto in Scene.lua.
  persist = 0,
  onSceneEnter = 0,  -- persistent entities: called as ( sceneName, spawnName ) after arriving in a new scene
  onDespawn = 0,     -- called just before a scene change (or Scene.removeForever) despawns the entity
  -- door = { to = "house", spawn = "entry", w = 4, h = 6, on = "touch" | "press", transition = "fade",
  --          duration = 0.6, music = "crossfade" | "cut" | "keep" }
  -- An invisible trigger zone w x h cells (from its pos): when the player (a hero collider) walks into it ("touch",
  -- the default) or presses E/Space inside it ("press"), Scene.goto( to, { spawn = spawn, ... } ). Scene files set
  -- each door's fields on the instance: { genome = "door", pos = { 150, 36 }, door = { to = "house", spawn = "entry" } }.
  door = function(entity, gene, tbl)
    local d = gene.val
    checkType( d, "table" )
    if not d.to then error( "door gene needs to = \"<scene name>\"" ) end
    local box = ffi.C.World_getBoxes().arr[entity]
    box.size.w, box.size.h = ( d.w or 4 ) * 256, ( d.h or 6 ) * 256
    ffi.C.addCollider( entity, d.layer or 0, COLL.door, 0, 0, 0, 0 )
    Scene.armDoor( tbl )
  end,
  sprite2d = 0,  -- read by Genome.new: the sprite pixel modes use instead of img/sprite (ASCII keeps its own art)
  -- sfx = { soundfont = "sfx", program = 120, sounds = { collect = 72 } }: the genome's own sound effects, played
  -- with self:sound( "collect" ) (scripts/Sound.lua). Resolved (and the SoundFont loaded, once per run) at spawn.
  sfx = function( entity, gene ) require( "Sound" ).prepare( gene.val ) end,
  -- timer = 1500 or { ms = 1500, rep = true, type = 0 }: starts with the entity; each time-out calls
  -- self:onTimer( timerId, type ). (A once-only timer: rep = false.) Despawning stops it.
  timer = function( entity, gene, tbl ) startTimer( entity, gene.val, tbl ) end,
  timerId = 0,    -- set by the timer gene
  params = 0,     -- per-instance parameters a scene / the Blender add-on may set (tools/blender; README, "Genomes")
  body = 0,       -- 3D physics body (Genome.spawn3d); 2D/2.5D entities ignore it
  shadow = 0,     -- 3D blob shadow: false = none, a radius (m) or { radius =, alpha = } (Genome.spawn3d; default on)
  genomeName = 0, -- set by Genome.get
  player = 0,     -- 3D: the persistent body put on the next scene's spawn point (Genome.spawn3d)
  key = 0,        -- the instance's saved-state key (Scene.keyOf)
  onSpawn = 0,  -- do nothing; called by Scene once the entity is placed
  onUncollision = 0,  -- do nothing, just allow it to exist
  spriteData = 0,  -- do nothing; set by the sprite loader
  onTimeout = 0,  -- do nothing, just allow it to exist
  anim = function(entity, gene)
    --TODO insert into world anim array
  end,
  vel = function(entity, gene)
    --TODO insert into world vel array
  end,
  name = 0,  -- do nothing, just allow it to exist
  lines = 0,  -- what an NPC says when talked to (see Dialogue.lua); read by its genome
  onInput = function(entity, gene) end,  -- do nothing, just allow it to exist
  onCollision = function(entity, gene) end,  -- do nothing, just allow it to exist
  onTimer = 0,  -- the timer gene's (or any createTimer's) time-outs: self:onTimer( timerId, type )
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
  if not loader and type( self.val ) == "function" then return end  -- genome-specific methods are fine
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
-- overrides: fields a scene sets on this instance (e.g. a door's target), copied over the genome's own.
Genome.new = function( name, overrides )
  local tbl = Genome.get( name ) -- tbl is the table of type-value pairs of genes (e.g. { img = "shopkeeper", ... })
  Genome.merge( tbl, overrides )
  -- sprite2d = "<aseprite sprite>": pixel modes draw this instead of the img/sprite gene, so a genome can keep
  -- hand-made ASCII art for the ASCII mode and still have pixel art in 2D/3D.
  if tbl.sprite2d and Sprite.renderMode() ~= "ascii" then
    tbl.sprite, tbl.img = tbl.sprite2d, nil
  end
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

-- What every genome instance can do (its table's metatable), whatever the render mode:
--   self:sound( "collect" [, vel] )   one of the genome's own sounds (its sfx gene); 3D bodies play it where they are
Genome.methods = {}
Genome.instanceMeta = { __index = Genome.methods }
function Genome.methods:sound( name, vel )
  local Sound = require( "Sound" )
  if self.pos and type( self.pos ) == "function" then  -- a 3D body (World3D.Body): positional
    local x, y, z = self:pos()
    return Sound.playFor( self, name, vel, x, y, z )
  end
  return Sound.playFor( self, name, vel )
end
-- Timers beyond the timer gene: onTimer( self, timerId, type ) gets them; type tells yours apart.
--   self:after( ms, type )   once        self:every( ms, type )   repeatedly        self:stopTimer( id )
function Genome.methods:after( ms, ttype ) return ffi.C.createTimer( ms, self.id, ttype or 0, false, ffi.C.TO_BRIDGE ) end
function Genome.methods:every( ms, ttype ) return ffi.C.createTimer( ms, self.id, ttype or 0, true, ffi.C.TO_BRIDGE ) end
function Genome.methods:stopTimer( id ) ffi.C.stopTimer( id ) end

-- Returns a fresh copy of the genome's table so several instances (e.g. coins) each keep their own state.
function Genome.get(genomeName)
  checkType(genomeName, "string")
  local proto = require("./"..gameName.."/genome/"..genomeName)
  local tbl = {}
  for k, v in pairs( proto ) do tbl[k] = v end
  tbl.genomeName = genomeName
  return setmetatable( tbl, Genome.instanceMeta )
end

-- Per-instance parameters over a genome's own fields (a scene's entry, a Blender placement): nested tables merge
-- ({ door = { to = "bonus" } } keeps the genome's door.radius), lists (a path) replace. The genome's tables are
-- copied, never changed, so other instances keep the defaults.
local function isList( t ) return t[1] ~= nil or next( t ) == nil end
function Genome.merge( tbl, overrides )
  for k, v in pairs( overrides or {} ) do
    if type( v ) == "table" and type( tbl[k] ) == "table" and not isList( v ) and not getmetatable( v ) then
      local copy = {}
      for kk, vv in pairs( tbl[k] ) do copy[kk] = vv end
      tbl[k] = Genome.merge( copy, v )
    else
      tbl[k] = v
    end
  end
  return tbl
end

Genome.startTimer = startTimer

local entityNamed  -- (MOTION, below)

-- Entry fields that are about where/how it's placed, not genome fields.
local PLACEMENT = { genome = true, pos = true, focus = true, key = true, yaw = true, persist = true }
Genome.PLACEMENT = PLACEMENT

--------------------
---- 3D GENOMES (true-3D scenes, World3D.lua)
--------------------
-- The same genome files, spawned as World3D bodies where a 3D level (or scene) places them:
--   model  = "coin3d"                             drawn as genome/body/model/coin3d (tools/blender exports it)
--   body   = { kind = "kinematic" | "character" | "box", radius, height, half = { x, y, z }, solid, pitch, yaw,
--              modelYaw, scale, gravity, jumpSpeed, accel, airAccel, maxSlope, step }   (no kind: drawn only)
--   coll   = { type = "coin" }                    its collision type (Genome.COLL): overlaps call onCollision
--   motion = { type = "spin", speed = 180, bob = 0.12, bobHz = 0.8 }           turns in place (and bobs)
--            { type = "linear", speed = 2, path = { {0,0,0}, {0,3,0} }, pingpong = true }  (relative to its spot)
--            { type = "orbital", radius = 2, speed = 45, around = "<genome>" | { x, y, z }, bob, bobHz }
--            { type = "follow", target = "hero", offset = { x, y, z }, rate = 6 }
--   door   = { to = "bonus", spawn = "entry", radius = 1, height = 2.5, on = "touch" | "press" }   a scene change
--   body.turnRate = 14                          turns to face the way it moves (1/s), e.g. after self:move( r, f )
--   killY = -15                                   below it: onFallOut( self ); landing: onLand( self )
--   listen = true                                 gets every onInput( self, key, pressed, dx, dy ), focused or not
--   onLevelReady( self )                          after the whole level is placed (find another placed genome)
--   sfx, timer, persist, onSpawn, onCollision, onUncollision, onTimer, onInput, onSceneEnter, onDespawn as in 2D.
-- entry = { genome = "coin", pos = { x, y, z }, yaw = <deg>, key = "<saved-state key>", focus = true (the player),
--           persist = true, <any genome field, e.g. door = { to = "bonus" } or motion = { path = {...} }> }.
-- Returns the body (its table is the instance: genome fields + World3D.Body methods + self:sound).
function Genome.spawn3d( name, entry )
  local W = require( "World3D" )
  entry = entry or {}
  local tbl = Genome.get( name )
  local over = {}
  for k, v in pairs( entry ) do if not PLACEMENT[k] then over[k] = v end end
  Genome.merge( tbl, over )
  local b = tbl.body or {}
  local p = entry.pos or { 0, 0, 0 }
  local collType = 0
  if type( tbl.coll ) == "table" then
    local t = tbl.coll.type or "solid"
    collType = type( t ) == "number" and t or COLL[t] or error( "Genome '"..name.."': unknown collision type '"..tostring( t ).."'" )
  end
  local kind = b.kind
  if tbl.door then  -- a trigger zone (unless the genome gives it a body of its own)
    collType = COLL.door
    kind = kind or "kinematic"
  end
  local persist = entry.persist or tbl.persist
  local player = entry.focus or tbl.player
  local body = W.Body.new{ kind = kind, x = p[1], y = p[2], z = p[3], yaw = ( entry.yaw or 0 ) + ( b.yaw or 0 ),
    pitch = b.pitch, radius = b.radius or ( tbl.door and tbl.door.radius ), height = b.height or ( tbl.door and tbl.door.height ),
    half = b.half, solid = b.solid, collType = collType, model = tbl.model, modelYaw = b.modelYaw, scale = b.scale,
    image = b.image, gravity = b.gravity, jumpSpeed = b.jumpSpeed, accel = b.accel, airAccel = b.airAccel,
    maxSlope = b.maxSlope, step = b.step, turnRate = b.turnRate, persist = persist, player = player,
    shadow = tbl.shadow }
  for k, v in pairs( tbl ) do if body[k] == nil then rawset( body, k, v ) end end
  body.key = entry.key or ( name.."@"..p[1]..","..p[2]..","..p[3] )
  body.spawnPos = { p[1], p[2], p[3] }
  register( body.id, body )
  if tbl.listen then listenInput( body.id ) end  -- every input event, focused or not (e.g. a camera)
  Genome.byName[name] = Genome.byName[name] or body.id
  if persist then W.persistentByName[name] = body end
  Scene.withScope( not persist, function()
    if tbl.sfx then require( "Sound" ).prepare( tbl.sfx ) end
    if tbl.door then Scene.armDoor( body ) end
    if tbl.timer then Genome.startTimer( body.id, tbl.timer, body ) end
    if tbl.motion then Genome.startMotion3d( body ) end
    if body.onSpawn then body:onSpawn() end
  end )
  return body
end

function Genome.startMotion3d( body )
  local m = body.motion
  checkType( m, "table" )
  local p = body.spawnPos
  if m.type == "spin" then
    if m.bob and m.bob ~= 0 then body:orbit{ center = p, radius = 0, speed = 0, bob = m.bob, bobHz = m.bobHz } end
    body:spin( m.speed or 90 )
  elseif m.type == "linear" then
    local pts = {}
    for i, q in ipairs( m.path or {} ) do
      pts[i] = m.absolute and q or { p[1] + q[1], p[2] + q[2], p[3] + q[3] }
    end
    if #pts > 0 then body:path( pts, m.speed or 2, m.pingpong and true or false ) end
  elseif m.type == "orbital" then
    local around = type( m.around ) == "string" and entityNamed( m.around, "orbit" ) or nil
    body:orbit{ center = type( m.around ) == "table" and m.around or p, around = around, radius = m.radius,
                speed = m.speed, phase = m.phase, bob = m.bob, bobHz = m.bobHz }
  elseif m.type == "follow" then
    body:follow( entityNamed( m.target, "follow" ), m )
  else
    error( "motion: unknown 3D type '"..tostring( m.type ).."' (spin, linear, orbital or follow)" )
  end
end

-- TODO call this from the loader to put a different position, dialogue, etc. on every instance of the same genome (e.g. shopkeepers w/ different items)
function Genome.inject(entity, tbl)
  -- TODO
end

Genome.COLL = COLL

--------------------
---- MOTION
--------------------
-- A genome declares how it moves with a motion gene (Kinematics, src/c/Kinematics.cpp). Units: cells (a sprite
-- pixel is 2 columns wide, 1 row tall), seconds, degrees. Paths are relative to where the entity spawns unless
-- absolute = true. Entities are named by genome ("hero"); the first one spawned with that genome is used.
--   motion = { type = "linear", speed = 8, path = { {0,0}, {30,0} }, pingpong = true }   -- patrol (cells/s)
--   motion = { type = "orbital", around = "horse", radius = 4, speed = 120, phase = 0 }  -- or around = {x, y}
--            radius is in rows (the orbit is 2 * radius columns wide so it looks round); rx/ry set an ellipse
--   motion = { type = "combo", radius = 3, speed = 180, travel = 6, path = { {0,0}, {40,0} }, pingpong = true }
--            an orbit (deg/s) whose center travels the path (cells/s)
--   motion = { type = "follow", target = "hero", mode = "lerp", rate = 4, offset = {6, -4}, bob = 0.5, bobHz = 1.2 }
--            eases toward the target's center + offset (the offset's x flips to stay behind it as it turns, unless
--            mirror = false): each frame pos += (goal - pos) * (1 - exp(-rate * dt)); mode = "hard" snaps.
--            deadzone = {x, y} lets the target roam before following. The camera uses the same options:
--            Camera.track( id, opts ) (scripts/bicycle.lua).
Genome.byName = {}  -- genome name -> entity id of the first instance spawned

entityNamed = function( ref, what )
  if type( ref ) == "number" then return ref end
  local id = Genome.byName[ref]
  if not id then error( "motion: no entity '"..tostring( ref ).."' to "..what ) end
  return id
end

function Genome.startMotion( instance )
  local m = instance.tbl.motion
  if not m then return end
  checkType( m, "table" )
  local id = instance.id
  local box = ffi.C.World_getBoxes().arr[id]
  local x0, y0 = box.pos.x / 256, box.pos.y / 256
  local function addPath( path, cx, cy )
    for _, p in ipairs( path or {} ) do
      if m.absolute then ffi.C.motionWaypoint( id, p[1], p[2] )
      else ffi.C.motionWaypoint( id, cx + p[1], cy + p[2] ) end
    end
  end
  local rows = m.radius or 3
  local rx, ry = m.rx or rows * 2, m.ry or rows
  if m.type == "linear" then
    ffi.C.motionLinear( id, m.speed or 6, m.pingpong and true or false )
    addPath( m.path, x0, y0 )
  elseif m.type == "orbital" then
    if type( m.around ) == "table" then
      ffi.C.motionOrbit( id, 0, m.around[1], m.around[2], rx, ry, m.speed or 90, m.phase or 0 )
    else
      ffi.C.motionOrbit( id, entityNamed( m.around, "orbit" ), 0, 0, rx, ry, m.speed or 90, m.phase or 0 )
    end
  elseif m.type == "combo" then
    ffi.C.motionCombo( id, rx, ry, m.speed or 180, m.phase or 0, m.travel or 6, m.pingpong and true or false )
    local cx, cy = x0 + box.size.w / 512, y0 + box.size.h / 512  -- the path moves the orbit's center
    addPath( m.path, cx, cy )
  elseif m.type == "follow" then
    -- Same options as track() / Camera.track() in scripts/bicycle.lua (mode, rate, offset, deadzone, mirror, bob).
    local opts = {}
    for k, v in pairs( m ) do opts[k] = v end
    opts.offset = opts.offset or { 6, -3 }
    if opts.mirror == nil then opts.mirror = true end
    track( id, entityNamed( m.target, "follow" ), opts )
  else
    error( "motion: unknown type '"..tostring( m.type ).."' (linear, orbital, combo or follow)" )
  end
end

Genome.Sprite = Sprite
Genome.Animation = Animation

return Genome
