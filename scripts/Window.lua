require("bicycle")

Window = {}
Window = {}
Window.__index = Window
Window.__newindex = function() error( "You can't create new attributes for Window." ) end

-- pop() causes bicycle::pop() to return the ID of the next top window on the stack after current one pops.
function Window.pop(id)
  focusOn( ffi.C.popWindow() )  -- focuses on table before this one
  unregister(id)
end

function Window.register(entityId, entityData)
  register(entityId, entityData)  -- register entity into bicycle's global entities table
end

function Window.focusOnEntity(entityId)
  focusOn(entityId)
end


