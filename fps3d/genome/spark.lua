-- A hit spark: lingers a moment where a shot hit, then goes away.
local spark = {
  model = "spark3d",
  timer = { ms = 400, rep = false },
}
function spark:onTimer()
  self:remove()
end
return spark
