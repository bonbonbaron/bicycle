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
    pos.y = pos.y + 1
  elseif input == ffi.C.S then
    pos.y = pos.y - 1
  end
  print(self.id)
end

return horse
