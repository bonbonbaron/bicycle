require("Window")

-- Layer etc. aren't going to be needed anywhere else, so I'll put them all in here.

-----------------------------------
---             LAYER         -----
-----------------------------------

Layer = {}
Layer.__index = Layer
Layer.__newindex = function() error( "You can't create new attributes for Layer." ) end
Layer.new = function()
  local id = ffi.C.pushScene()
  local instance = { id=id, cbs = {} }
  setmetatable( instance, Layer )
  Window.push(id, instance)
  return instance
end

function Layer:add( str, cb )
  if type(cb) == "function" then
    ffi.C.addItem( str )
    table.insert( self.cbs, cb )  -- index of cursor corresponds to selection
  end
end

-----------------------------------
---             SCENE         -----
-----------------------------------

Scene = {}
Scene.__index = Scene
Scene.__newindex = function() error( "You can't create new attributes for Scene." ) end
Scene.new = function()
  local id = ffi.C.pushScene()
  local instance = { id=id, cbs = {} }
  setmetatable( instance, Scene )
  setmetatable( Scene, Window )
  Window.push(id, instance)
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



