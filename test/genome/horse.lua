require('bicycle')
local horse = { img="horse" }
function horse:onInput(input)
  local rects = ffi.C.World_getRects()
  pos = rects.arr[self.id].pos -- hopefully this works
  if input == ffi.C.A then
    pos.x = pos.x - 1
  elseif input == ffi.C.D then
    pos.x = pos.x + 1
  elseif input == ffi.C.W then
    pos.y = pos.y - 1
  elseif input == ffi.C.S then
    pos.y = pos.y + 1
  elseif input == ffi.C.Q then
    ffi.C.popWindow()
  elseif input == ffi.C.T then
    ffi.C.pushDialogue( "NeeiiiighhhhH!!!", pos.x, pos.y - 1, 30, 3 ) -- TODO 1) steal input from horse,
  end
end

return horse
