require("Window")

Dialogue = {}
Dialogue.__index = Dialogue
Dialogue.__newindex = function() error( "You can't create new attributes for Dialogue." ) end
Dialogue.new = function( text, x, y, w, h )
  local id = ffi.C.pushDialogue( text, x, y, w, h )
  local instance = { id=id, x=x, y=y, w=w, h=h, cbs = {} }
  setmetatable( instance, Dialogue )
  setmetatable( Dialogue, Window )
  Window.push(id, instance)
  return instance
end
function Dialogue:onInput( input )
  if input == ffi.C.Space or input == ffi.C.B then
    Window.pop(self.id)
  end
end
