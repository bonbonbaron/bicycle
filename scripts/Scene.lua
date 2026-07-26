require("Window")
require("io")
require("table")

-- Layer etc. aren't going to be needed anywhere else, so I'll put them all in here.

local Scene = {}
Scene.__index = Scene
Scene.__newindex = function() error( "You can't create new attributes for Scene." ) end

function Scene:getFileIter(path)
  -- Open a pipe to read ("r") from the command.
  local handle = io.popen("ls "..path)
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
      local pos = ffi.new("Position", { x=instance.pos[1], y=instance.pos[2], z=instance.pos[3] or 0 } )
      ffi.C.setPos( genome.id, pos )
    end
  end
end

function Scene:readLayers(Genome)
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
  setmetatable( instance, Scene )  -- inherit Scene
  setmetatable( Scene, Window )    -- have Scene in turn inherit Window (its supertype)
  -- Push window to bicycle's internal stack of window
  Window.register(id, instance)
  instance:readLayers(Genome)
  return instance
end

-- TODO function Scene::remove(), good for item menus in RPGs

function Scene:onInput( input )
  if input == ffi.C.Q then
    self.pop(self.id)
  end
end

return Scene
