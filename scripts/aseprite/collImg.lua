local sprite = app.sprite

sprite.properties.colorCollisionTypes = sprite.properties.colorCollisionTypes or {DEFAULT = ""}
sprite.properties.colorToCollisionType = sprite.properties.colorToCollisionType or {DEFAULT = ""}
sprite.properties.tileCollisionTypes = sprite.properties.tileCollisionTypes or {DEFAULT = ""}           -- .. and and this guy is not.
sprite.properties.tileIdxToCollisionType = sprite.properties.tileIdxToCollisionType or {DEFAULT = ""}   -- I think this guy is being persisted... 
sprite.properties.eventKeys = sprite.properties.eventKeys or {DEFAULT=""}

local function pkv(m)
  for k, v in pairs(m) do
    print("key "..k.." --> "..tostring(v))
  end
end

local dlg

local function getKey(datatype)
  datatype = datatype or "color"
  if datatype == "color" then
      return app.pixelColor.rgba(app.fgColor.red, app.fgColor.green, app.fgColor.blue, app.fgColor.alpha)
  elseif datatype == "tile" then
    return app.fgTile
  end
  return "INVALID_DATATYPE"
end


local function copyList(src)
  local dst = {}
  if type(src) == "table" then
    local ok, err = pcall(function()
      for i, v in ipairs(src) do
        dst[i] = v
      end
    end)
    if not ok then
      print("copyList: Error occurred while copying list: " .. tostring(err))
    end
  end
  return dst
end

local function copyMap(src)
  local dst = {}
  if type(src) ~= "table" then
    return dst
  end

  local ok, err = pcall(function()
    for k, v in pairs(src) do
      if type(k) == "number" then
        dst[tostring(k)] = tostring(v)
      elseif type(k) == "string" then
        dst[k] = tostring(v)
      end
    end  -- the debugger shows that the "invalid key to 'next'" error occurs here, but I don't know why. Pcall does NOT catch it.
  end)

  if not ok then
    print("copyMap: invalid property table; ignoring it: " .. tostring(err))
    return {}
  end

  return dst
end

local printed = false
local function getProperties(datatype)
  datatype = datatype or "color"
  if datatype == "color" then
    return app.sprite.properties.colorCollisionTypes, app.sprite.properties.colorToCollisionType
  elseif datatype == "tile" then
    return app.sprite.properties.tileCollisionTypes, app.sprite.properties.tileIdxToCollisionType
  end
  return nil, nil
end

local function resetProperties(datatype)
  datatype = datatype or "color"
  if datatype == "color" then
    sprite.properties.colorCollisionTypes = {DEFAULT = ""}
    sprite.properties.colorToCollisionType =  {DEFAULT = ""}
  elseif datatype == "tile" then
    sprite.properties.tileCollisionTypes =  {DEFAULT = ""}
    sprite.properties.tileIdxToCollisionType =  {DEFAULT = ""}
  end
end

local function loadState(datatype)
  datatype = datatype or "color"
  return getProperties(datatype)
end

local function sanitizeTypes(tbl)
  local replacementTbl = {}
  for k, v in ipairs(tbl) do
    replacementTbl[k] = v
  end
  if #replacementTbl then
    return replacementTbl
  else
    return nil
  end
end

local function sanitizeMap(tbl)
  local replacementTbl = {}
  for k, v in pairs(tbl) do
    replacementTbl[tostring(k)] = v
  end
  if #replacementTbl then
    return replacementTbl
  else
    return nil
  end
end

local function saveState(types, map, datatype)
  datatype = datatype or "color"
  -- Aseprite wants properties' tables to have strictly string keys.
  types = sanitizeTypes(types) or {DEFAULT = ""}
  map = sanitizeMap(map) or {DEFAULT = ""}
  app.transaction("Update collision types", function()
    if datatype == "color" then
      print("color-based collision:")
      pkv(map)
      app.sprite.properties.colorCollisionTypes = types
      app.sprite.properties.colorToCollisionType = map
    elseif datatype == "tile" then
      print("tile-based collision:")
      pkv(map)
      app.sprite.properties.tileCollisionTypes = types
      app.sprite.properties.tileIdxToCollisionType = map
    end
  end)
end

local function typeForColor(map, color)
  local key = app.pixelColor.rgba(color.red, color.green, color.blue, color.alpha)
  if key == nil then
    return "(none)"
  end
  return map[key] or map[tostring(key)] or "(unassigned)"
end

local function typeForTile(map, tileIdx)
  if tileIdx == nil then
    return "(none)"
  end
  return map[tileIdx] or map[tostring(tileIdx)] or "(unassigned)"
end

local function hasType(types, name)
  local ok, err = pcall(function()
  for _, existing in ipairs(types) do
      if existing == name then
        return true
      end
    end
  end)
  if not ok then
    print("hasType: Error occurred while checking type: " .. tostring(err))
  end
  return false
end

local function statusText(map, datatype)
  datatype = datatype or "color"
  if datatype == "color" then
    local c = app.fgColor
    if c == nil then
      return "No foreground color is available."
    end
    local rgba = app.pixelColor.rgba(c.red, c.green, c.blue, c.alpha)
    local collision = typeForColor(map, c)
    return string.format(
      "Foreground color #%08X  —  collision type: %s",
      rgba or 0,
      collision
    )
  else
    local t = app.fgTile
    if t == nil then
      return "No foreground color is available."
    end
    local collision = typeForTile(map, t)
    return string.format(
      "Foreground tile %d  —  collision type: %s",
      t or -1,
      collision
    )
  end
end

local function closeDialog()
  if dlg ~= nil then
    dlg:close()
    dlg = nil
  end
end

local function showDialog(datatype)
  datatype = datatype or "color"
  if datatype ~= "color" and datatype ~= "tile" then
    app.alert("You should only be dealing in either tiles or colors. Pick one.")
    return
  end
  local sprite = app.sprite
  if sprite == nil then
    app.alert("Open a sprite before assigning collision types.")
    return
  end

  local types, map = loadState(datatype)
  local selected = types[1]

  closeDialog()

  dlg = Dialog{
    title = "Assign collision type to "..datatype,
    onclose = function()
      dlg = nil
    end
  }

  dlg:label{
    id = "status",
    text = statusText(map,datatype)
  }

  dlg:combobox{
    id = "type",
    label = "Collision type:",
    option = selected,
    options = types
  }

  dlg:button{
    id = "assign",
    text = "Assign to foreground "..datatype,
    focus = true,
    onclick = function()
      if app.sprite == nil then
        app.alert("No active sprite.")
        return
      end
      
      local key = getKey(datatype)
      if key == nil then
        app.alert("No foreground "..datatype.." is available.")
        return
      end
      
      local chosen = dlg.data.type
      if chosen == nil or chosen == "" then
        app.alert("Select a collision type first.")
        return
      end
      
      types, map = loadState(datatype)
      map[key] = chosen
      print("setting "..key.." to "..chosen)
      saveState(types, map, datatype)
      dlg:modify{ id = "status", text = statusText(map, datatype) }
      app.tip(string.format("Color #%08X assigned to \"%s\"", key, chosen))
    end
  }
  
  dlg:separator{ text = "New collision type" }

  dlg:entry{
    id = "newType",
    label = "Name:",
    text = ""
  }

  -- button to adda  new collision type
  dlg:button{
    id = "addType",
    text = "Add "..datatype.." collision type",
    onclick = function()
      local current = app.sprite
      if current == nil then
        app.alert("No active sprite.")
        return
      end

      local name = dlg.data.newType
      if type(name) ~= "string" then
        name = ""
      end
      name = name:match("^%s*(.-)%s*$") or ""
      if name == "" then
        app.alert("Enter a name for new "..datatype.." collision type.")
        return
      end

      types, map = loadState(datatype)
      if hasType(types, name) then
        app.alert(string.format("Collision type \"%s\" already exists.", name))
        return
      end

      table.insert(types, name)
      saveState(types, map, datatype)
      app.tip(string.format("Added "..datatype.." collision type \"%s\"", name))
      showDialog(datatype)
    end
  }  -- button to adda  new datatype

  dlg:separator()

  dlg:button{
    id = "clear",
    text = "Clear assignments",
    onclick = function()
      local current = app.sprite
      if current == nil then
        app.alert("No active sprite.")
        return
      end

      saveState({}, {}, datatype)
      resetProperties(datatype)
      app.tip("Cleared all "..datatype.." collision type assignments.")
    end
  }

  dlg:button{
    id = "close",
    text = "Close",
    onclick = function()
      closeDialog()
    end
  }

  dlg:show{ wait = false }
end  -- showDialog()

local function isValid()
  return app.layer and ( app.layer.name == "collision" and app.fgColor ) or (app.layer.isTilemap and app.fgTile)
end

local function getDatatype()
  if isValid() then
    if app.layer.name == "collision" and app.fgColor then
      return "color"
    elseif app.layer.isTilemap and app.fgTile then
      return "tile"
    end
  end
  return "INVALID_DATATYPE"
end

local goodMsgStrs = {
  color = "Collision type for color #%08X is %s",
  tile = "Collision type for tile idx %d is %s" 
}
local badMsgStrs = {
  color = "No collision type set for color #%08X",
  tile = "No collision type set for tile %d"
}

local function showCollisionMapping()
  if not isValid() then
    return
  end
  local sprite = app.sprite
  local datatype = getDatatype()
  -- Don't react to foregroud color changes in tilemap layer.
  if datatype == "tile" then
    return
  end
  if sprite == nil then
    return
  end

  local _, map = loadState(datatype)
  if map == nil then
    app.tip(datatype.." map is empty; nothing to show.")
    return
  end
  local key = getKey()
  local collisionType = map[key] or map[tostring(key)]
  if collisionType then
    app.tip(string.format(
      goodMsgStrs[datatype],
      key,
      collisionType
    ))
  else
    app.tip(string.format(
      badMsgStrs[datatype],
      key
    ))
  end
end

local eventKey = app.events:on("fgcolorchange", showCollisionMapping)
table.insert(sprite.properties.eventKeys, eventKey)

if isValid() then
  local datatype = getDatatype()
  showDialog(datatype)
end

print("final color-to-coll state:")
pkv(app.sprite.properties.colorToCollisionType)