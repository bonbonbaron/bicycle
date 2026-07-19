require("bicycle")

Window = {}
Window = {}
Window.__index = Window
Window.__newindex = function() error( "You can't create new attributes for Window." ) end

function Window.pop(id)
  focusOn( ffi.C.popWindow() )  -- focuses on table before this one
  unregister(id)
end

function Window.push(id, entity)
  register(id, entity)
  focusOn(id)
end



