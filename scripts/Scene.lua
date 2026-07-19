require("Window")
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

function Scene:readBgs()
  handle = self:getFileIter( "./test/scene/"..self.name.."/bg/*.bg" )
  -- Put the files found above into a list.
  numFiles = 0
  for fp in handle:lines() do
    local file = io.open( fp, "r" )
    local bgStr = file:read("*a")
    file:close()
    numFiles = numFiles + 1
    ffi.C.addLayer( bgStr )
  end
  handle:close()
  if numFiles == 0 then error("Found no *.l files in ./test/scene/"..self.name.."/bg") end
end

-- Lua is how I'm going to do everything I ever dreamed of that was nightmarish in all other directions:
--    1. position genome multiple times in the layer
--    2. indicate the animation they start with
--    3. complicated logic when loading a scene (where you start, tileset, song, etc based on global game state)

function Scene:readFgs()
  handle = self:getFileIter( "./test/scene/"..self.name.."/fg/*.lua" )
  -- Put the files found above into a list.
  numFiles = 0
  for fp in handle:lines() do
    local file = io.open( fp, "r" )
    local contents = file:read("*a")
    file:close()
    numFiles = numFiles + 1
    ffi.C.addLayer( contents )
  end
  handle:close()
  if numFiles == 0 then error("Found no *.l files in ./test/scene/"..self.name.."/fg") end
end

function Scene:readLayers()
  self:readBgs()
  self:readFgs()
end

function Scene.new( name )
  if type(name) ~= "string" then error( "Scene needs a name bro." ) end
  local id = ffi.C.pushScene()
  local instance = { name=name, id=id, cbs = {} }
  setmetatable( instance, Scene )
  setmetatable( Scene, Window )
  Window.push(id, instance)
  instance:readLayers()
  return instance
end


function Scene:add( str, cb )
  if type(cb) == "function" then
    ffi.C.addItem( str )
    table.insert( self.cbs, cb )  -- index of cursor corresponds to selection
  end
end

-- TODO function Scene::remove(), good for item menus in RPGs

function Scene:onInput( input )
  if input == ffi.C.Q then
    self.pop(self.id)
  end
end



