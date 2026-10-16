require("Window")
require("io")
require("table")

-- Layer etc. aren't going to be needed anywhere else, so I'll put them all in here.

local Scene = {}
Scene.__index = Scene
Scene.__newindex = function() error( "You can't create new attributes for Scene." ) end

function Scene:getFileIter(path)
  -- Open a pipe to read ("r") from the command.
  local isWindows = package.config:sub(1, 1) == "\\"
  local handle = io.popen(isWindows and ('dir /b "'..path:gsub("/", "\\")..'"') or ("ls "..path))
  if not handle then
    error("failed to execute ls for "..self.name)
  end
  return handle
end

function dbgp( tbl )
  for key, val in pairs(tbl) do
    print( tostring(key)..": "..tostring(val) )  -- tall order...
  end
  io.read()
end

-- BG will be asterisked. You have to specify which layer you want your entities on.
function Scene:getBg( path )
  local bg = {}
  checkType(path ,"string")
  local packed = Asset.list( path:gsub( "/%*%.txt$", "" ) )
  if packed then
    for _, fp in ipairs( packed ) do
      if fp:match( "%.txt$" ) then table.insert( bg, Asset.read( fp ) ) end
    end
    if #bg == 0 then error("Found no files in "..path) end
    return bg
  end
  handle = self:getFileIter( path )
  -- Process each file found above with callback argument.
  numFiles = 0
  for fp in handle:lines() do
    local file = io.open( fp, "r" )
    local contents = file:read("*a")
    table.insert( bg, contents )
    file:close()
    numFiles = numFiles + 1
  end
  handle:close()
  if numFiles == 0 then error("Found no files in "..path) end
  return bg
end

-- \ TODO revive BG
-- TODO impl FG

-- This adds the genomes of all the kk
function Scene:addFg( Genome, fg )
	checkType( fg, "table" )
  for _, instance in ipairs( fg ) do
    -- Express the genome of each entity.
    local genome = Genome.new( instance.genome )
    ffi.C.addFgEntity( genome.id )
    if instance.pos then
      local pos = ffi.new("Position", 
      { x=instance.pos[1],
        y=instance.pos[2],
        z=instance.pos[3] or 0 } )
      ffi.C.setPos( genome.id, pos )
      -- TODO keep a reference to the position in entity data for easy manipulation
      local boxes = ffi.C.World_getBoxes()
      genome.tbl.pos = boxes.arr[genome.id].pos -- hopefully this works
      genome.tbl.size = boxes.arr[genome.id].size
    end
    if instance.focus then
      focusOn( genome.id )
      ffi.C.focusCamOn( genome.id)
    end
    genome.tbl.id = genome.id

    register( genome.id, genome.tbl )
    if genome.tbl.onSpawn then genome.tbl:onSpawn() end
  end
end

-- bg is either an Aseprite background sprite exported by scripts/aseprite/sprite.lua (one tilemap layer per
-- background layer, each with its mode) or a directory of .txt layers under genome/scene/bg/.
-- fg[i] holds the entities that live in background layer i.
function Scene:readLayers(Genome)
  local Sprite = Genome.Sprite
  if Sprite.exists( self.bg ) then
    local spr = Sprite.load( self.bg )
    if spr.kind ~= Sprite.TILEMAP then error( "Scene background '"..self.bg.."' isn't a tilemap sprite." ) end
    for bgIdx, tm in ipairs( spr.layers ) do
      Sprite.addBgLayer( tm )
      if self.fg[bgIdx] then
        self:addFg( Genome, self.fg[bgIdx] )
      end
    end
    return
  end
  -- TODO later don't require backgrounds to allow for entities without 
  local bg = self:getBg( "./"..gameName.."/genome/scene/bg/"..self.bg.."/*.txt"  )
  local bgIdx = 1
  for _, layer in pairs(bg) do
    ffi.C.addBgLayer( layer )
    if self.fg[bgIdx] then
      self:addFg( Genome, self.fg[bgIdx] )  -- bg in the layer level specifies which bg these entities belong in
    end
    bgIdx = bgIdx + 1
  end
end

function Scene.new( tbl )
	checkType(tbl, "table")
  local id = ffi.C.pushScene()  -- bicycle::pushScene() returns an entity ID of that scene
  local instance = { name=tbl.name, id=tbl.id, bg=tbl.bg, fg=tbl.fg }
  local Genome = require("Genome")
  require("Sound").load( tbl.music, tbl.sfx )
  setmetatable( instance, Scene )  -- inherit Scene
  setmetatable( Scene, Window )    -- have Scene in turn inherit Window (its supertype)
  -- Push window to bicycle's internal stack of window
  Window.register(id, instance)
  instance:readLayers(Genome)
  instance:loadScenery3d(Genome)
  return instance
end

-- 3D mode: static scenery from tools/blender/export.py's scene3d.lua (ground, props). Characters in it are placed
-- by their genomes' model genes instead.
function Scene:loadScenery3d(Genome)
  if Genome.Sprite.renderMode() ~= "3d" then return end
  local dir = "./"..gameName.."/genome/"
  if not Asset.exists( dir.."models/scene3d.lua" ) then return end
  local scenery = Asset.dofile( dir.."models/scene3d.lua" )
  for _, obj in ipairs( scenery.objects ) do
    if obj.role ~= "character" and not ffi.C.addStaticModel( dir..obj.mesh..".bmesh" ) then
      error( "Couldn't load 3D scenery '"..obj.mesh.."' (build the 'assets' target)." )
    end
  end
end

-- TODO function Scene::remove(), good for item menus in RPGs

function Scene:onInput( input )
  if input == ffi.C.Q then
    self.pop(self.id)
  end
end

return Scene
