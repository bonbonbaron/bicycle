-- AutoTilePicker.lua
-- Automatically selects a tile from the active tileset based on
-- mouse position (under-cursor) or neighboring tiles (perimeter / bitmask).
-- Place in your Aseprite scripts folder and assign a shortcut.

local function getActiveTilemapInfo()
  local spr = app.sprite
  if not spr then return nil, "No active sprite" end

  local layer = app.layer
  if not layer or not layer.isTilemap then
    return nil, "Active layer is not a tilemap"
  end

  local tileset = layer.tileset
  if not tileset then return nil, "No tileset on active layer" end

  local frame = app.frame
  local cel = layer:cel(frame)
  if not cel then return nil, "No cel on current frame" end

  local tileSize = tileset.grid.tileSize
  local tw = math.max(1, math.abs(tileSize.width))
  local th = math.max(1, math.abs(tileSize.height))

  return {
    sprite = spr,
    layer = layer,
    tileset = tileset,
    cel = cel,
    image = cel.image,
    tileW = tw,
    tileH = th,
    origin = cel.position
  }
end

local function spriteToTileCoords(info, spriteX, spriteY)
  local lx = spriteX - info.origin.x
  local ly = spriteY - info.origin.y
  if lx < 0 or ly < 0 then return nil end

  local tx = math.floor(lx / info.tileW)
  local ty = math.floor(ly / info.tileH)

  if tx < 0 or ty < 0 or tx >= info.image.width or ty >= info.image.height then
    return nil
  end
  return tx, ty
end

local function getTileIndexAt(info, tx, ty)
  if tx < 0 or ty < 0 or tx >= info.image.width or ty >= info.image.height then
    return 0  -- treat out-of-bounds as empty
  end
  local entry = info.image:getPixel(tx, ty)
  return app.pixelColor.tileI(entry)
end

-- Classic 4-bit bitmask (N E S W). Extend to 8-bit if you need diagonals.
local function computeMask(info, tx, ty)
  local n = getTileIndexAt(info, tx,     ty - 1) > 0 and 1 or 0
  local e = getTileIndexAt(info, tx + 1, ty    ) > 0 and 2 or 0
  local s = getTileIndexAt(info, tx,     ty + 1) > 0 and 4 or 0
  local w = getTileIndexAt(info, tx - 1, ty    ) > 0 and 8 or 0
  return n + e + s + w   -- 0..15
end

-- Map mask → preferred tile index.
-- Adjust this table to match the order of your tileset.
-- Example assumes a common 16-tile layout (or the first 16 tiles of a larger set).
local MASK_TO_TILE = {
  [0]  = 0,   -- isolated / empty
  [1]  = 1,   -- N
  [2]  = 2,   -- E
  [3]  = 3,   -- N+E
  [4]  = 4,   -- S
  [5]  = 5,   -- N+S
  [6]  = 6,   -- E+S
  [7]  = 7,   -- N+E+S
  [8]  = 8,   -- W
  [9]  = 9,   -- N+W
  [10] = 10,  -- E+W
  [11] = 11,  -- N+E+W
  [12] = 12,  -- S+W
  [13] = 13,  -- N+S+W
  [14] = 14,  -- E+S+W
  [15] = 15,  -- full
}

local function pickTileUnderCursor()
  local info, err = getActiveTilemapInfo()
  if not info then
    app.alert(err)
    return
  end

  local mouse = app.editor.spritePos
  local tx, ty = spriteToTileCoords(info, mouse.x, mouse.y)
  if not tx then return end

  local entry = info.image:getPixel(tx, ty)
  local idx = app.pixelColor.tileI(entry)
  if idx >= 0 and idx < #info.tileset then
    app.fgTile = entry
    app.refresh()
  end
end

local function pickTileByPerimeter()
  local info, err = getActiveTilemapInfo()
  if not info then
    app.alert(err)
    return
  end

  local mouse = app.editor.spritePos
  local tx, ty = spriteToTileCoords(info, mouse.x, mouse.y)
  if not tx then return end

  local mask = computeMask(info, tx, ty)
  local preferred = MASK_TO_TILE[mask] or 0

  if preferred >= 0 and preferred < #info.tileset then
    -- Build a proper tile entry (index + flags). Flags = 0 for normal orientation.
    local entry = app.pixelColor.tile(preferred, 0)
    app.fgTile = entry
    app.refresh()
  end
end

----------------------------------------------------------------------
-- Simple dialog with two buttons + optional live timer
----------------------------------------------------------------------
local dlg = Dialog("Auto Tile Picker")
local timer = nil
local liveMode = false

dlg:button{
  id = "under",
  text = "Pick Under Cursor",
  onclick = function()
    pickTileUnderCursor()
  end
}

dlg:button{
  id = "perimeter",
  text = "Pick by Neighbors (Mask)",
  onclick = function()
    pickTileByPerimeter()
  end
}

dlg:check{
  id = "live",
  text = "Live update (while dialog open)",
  selected = false,
  onclick = function()
    liveMode = dlg.data.live
    if liveMode then
      if not timer then
        timer = Timer{
          interval = 0.05,   -- 20 Hz
          ontick = function()
            if app.sprite and app.layer and app.layer.isTilemap then
              -- Prefer perimeter mode when live is on; change to under-cursor if desired
              pickTileByPerimeter()
            end
          end
        }
      end
      timer:start()
    else
      if timer then timer:stop() end
    end
  end
}

dlg:separator()
dlg:label{ text = "Tip: Bind the two commands to hotkeys" }
dlg:label{ text = "or keep this dialog open for live picking." }
dlg:button{ id = "close", text = "Close", focus = true }

dlg:show{ wait = false }

-- Clean up timer when dialog is closed
dlg.onclose = function()
  if timer then
    timer:stop()
    timer = nil
  end
end