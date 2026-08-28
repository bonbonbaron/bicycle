function printKeys(t)
  for k, v in pairs(t) do
    print(k..": "..tostring(v))
  end
end

local spr = app.sprite
if not spr then return end

local layer = app.layer
if not layer or not layer.isTilemap then
  return app.alert("Active layer is not a tilemap.")
end

local ts = layer.tileset
local tileIdx = app.fgTile
print("tileIdx is "..tileIdx)
local tile = ts:tile(tileIdx)
-- printKeys(tile)
if not tile then
  return app.alert("Select a tile in the tileset first.")
end


local dlg = Dialog("Tile duration")
dlg:number{ id="ms", label="Duration (ms)", text=tostring(tile.properties.duration or 0) }
dlg:button{ id="ok", text="Apply" }
dlg:button{ id="clear", text="Clear" }
dlg:button{ text="Cancel" }
dlg:show()

if dlg.data.ok then
  local ms = tonumber(dlg.data.ms) or 0
  app.transaction("Set tile duration", function()
    if ms > 0 then
      tile.properties.duration = ms
    else
      tile.properties.duration = nil
    end
  end)
elseif dlg.data.clear then
  app.transaction("Clear tile duration", function()
    tile.properties.duration = nil
  end)
end