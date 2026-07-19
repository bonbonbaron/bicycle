require("Window")
require("Genome")
require("io")
require("table")

-- Layer etc. aren't going to be needed anywhere else, so I'll put them all in here.

Scene = {}
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

function Scene:process( path, processFunction )
  checkType(path ,"string")
  checkType(processFunction, "function")
  handle = self:getFileIter( path )
  -- Process each file found above with callback argument.
  numFiles = 0
  for fp in handle:lines() do
    local file = io.open( fp, "r" )
    local contents = file:read("*a")
    file:close()
    numFiles = numFiles + 1
    processFunction( self, contents )
  end
  handle:close()
  if numFiles == 0 then error("Found no [0-9].l files in ./"..gameName.."/scene/"..self.name.."/bg") end
end

function Scene:addBg( bgStr )
	checkType( bgStr, "string" )
  ffi.C.addBgLayer( bgStr )
end

function Scene:readLayers()
  -- TODO express genomes, which returns a list of entities. Determine the format.
  --
  self:process("./"..gameName.."/scene/"..self.name.."/bg/*.bg", self.addBg )
  -- self:process("./"..gameName.."/scene/"..self.name.."/fg/*.lua", self.a)
end

function Scene.new( name )
	checkType(name, "string")
  local id = ffi.C.pushScene()
  local instance = { name=name, id=id, cbs = {} }
  setmetatable( instance, Scene )
  setmetatable( Scene, Window )
  Window.push(id, instance)
  instance:readLayers()
  return instance
end

-- TODO function Scene::remove(), good for item menus in RPGs

function Scene:onInput( input )
  if input == ffi.C.Q then
    self.pop(self.id)
  end
end



