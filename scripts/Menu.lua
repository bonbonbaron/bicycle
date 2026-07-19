require("Window")

Menu = {}
Menu.__index = Menu
Menu.__newindex = function() error( "You can't create new attributes for Menu." ) end
Menu.new = function( x, y, w, h )
  local x = x or 0 
  local y = y or 0
  local w = w or 10
  local h = h or 10
  local id = ffi.C.pushTextMenu( x, y, w, h )
  local instance = { id=id, x=x, y=y, w=w, h=h, cbs = {} }
  setmetatable( instance, Menu )
  setmetatable( Menu, Window )
  Window.push(id, instance)
  return instance
end

function Menu:add( str, cb )
  if type(cb) == "function" then
    ffi.C.addItem( str )
    table.insert( self.cbs, cb )  -- index of cursor corresponds to selection
  end
end

-- TODO function Menu::remove(), good for item menus in RPGs

function Menu:onInput( input )
  if input == ffi.C.Space then
    selIdx = ffi.C.getSelection()
    self.cbs[selIdx+1]()
  elseif input == ffi.C.B then
    self.pop(self.id)
  end
end

