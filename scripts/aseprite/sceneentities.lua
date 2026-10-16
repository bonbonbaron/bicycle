-- The "entities" layer of a scene's background sprite (README, "Placing entities in Aseprite"): every entity of the
-- scene is a slice on the canvas, named "<genome>#<n>", whose properties hold
--   genome = "hero", fg = 4 (the scene's fg layer), extra = 'focus = true, persist = true' (more fields, as Lua)
-- The slice's top-left is the entity's position (1 sprite pixel = 2x1 cells) and, for doors, its size is the
-- door's w/h. A small outline on the "entities" layer marks each one. write() puts them all into the scene file
-- (genome/scene/<scene>.lua) between generated markers in each fg layer, keeping hand-written entries outside
-- the markers; running it again rewrites the same text (idempotent).
local current_file = debug.getinfo(1, "S").source:match("^@?(.*)")
local current_dir = current_file:match("^(.*[/\\])") or "./"
package.path = current_dir .. "?.lua;" .. package.path
local L = require("scenelib")

local E = {}
E.LAYER = "entities"
E.BEGIN = "-- BEGIN entities (generated from the entities layer of %s.aseprite: place them there)"
E.END = "-- END entities"

-- ---- Lua values <-> text ----
local function isIdent(k) return type(k) == "string" and k:match("^[%a_][%w_]*$") end
local function ser(v)
  local t = type(v)
  if t == "string" then return string.format("%q", v) end
  if t == "number" then
    if math.type and math.type(v) == "integer" then return tostring(v) end
    if v == math.floor(v) then return string.format("%d", v) end
    return tostring(v)
  end
  if t == "boolean" then return tostring(v) end
  if t == "table" then
    local parts, n = {}, #v
    for i = 1, n do parts[#parts + 1] = ser(v[i]) end
    local ks = {}
    for k in pairs(v) do
      if not (math.type and math.type(k) == "integer" and k >= 1 and k <= n) then ks[#ks + 1] = k end
    end
    table.sort(ks, function(a, b) return tostring(a) < tostring(b) end)
    for _, k in ipairs(ks) do
      parts[#parts + 1] = (isIdent(k) and k or ("[" .. ser(k) .. "]")) .. " = " .. ser(v[k])
    end
    return "{ " .. table.concat(parts, ", ") .. " }"
  end
  error("can't write a " .. t .. " into a scene file")
end
E.ser = ser

-- Fields of an entity other than genome/pos, as 'k = v, ...' (keys sorted, so the text is stable).
function E.extraText(entry)
  local ks = {}
  for k in pairs(entry) do if k ~= "genome" and k ~= "pos" then ks[#ks + 1] = k end end
  table.sort(ks)
  local parts = {}
  for _, k in ipairs(ks) do parts[#parts + 1] = k .. " = " .. ser(entry[k]) end
  return table.concat(parts, ", ")
end
function E.parseExtra(text)
  if not text or text:match("^%s*$") then return {} end
  local f = load("return { " .. text .. " }", "=extra", "t", {})
  if not f then error("bad entity fields: " .. text) end
  return f()
end

-- One scene-file line for an entity.
function E.entryText(genome, x, y, extra)
  local s = '{ genome = "' .. genome .. '", pos = { ' .. ser(x) .. ', ' .. ser(y) .. ' }'
  if extra and extra ~= "" then s = s .. ", " .. extra end
  return s .. " },"
end

-- ---- the sprite side ----
function E.layer(sprite, create)
  for _, l in ipairs(sprite.layers) do if l.name == E.LAYER then return l end end
  if not create then return nil end
  local l = sprite:newLayer()
  l.name = E.LAYER
  return l
end

-- The entity slices, in creation order: { slice, genome, fg, extra, x, y, w, h } (x/y/w/h in cells).
function E.list(sprite)
  local out = {}
  for _, s in ipairs(sprite.slices) do
    local p = s.properties
    if p and p.genome then
      local b = s.bounds
      out[#out + 1] = { slice = s, genome = p.genome, fg = p.fg or 1, extra = p.extra or "", order = p.order or 0,
                        x = b.x * 2, y = b.y, w = b.width * 2, h = b.height }
    end
  end
  -- Placement order (the order entities spawn in): properties.order, then name.
  table.sort(out, function(a, b)
    if a.order ~= b.order then return a.order < b.order end
    return a.slice.name < b.slice.name
  end)
  return out
end

local function nextName(sprite, genome)
  local used = {}
  for _, s in ipairs(sprite.slices) do used[s.name] = true end
  local n = 1
  while used[genome .. "#" .. n] do n = n + 1 end
  return genome .. "#" .. n
end

-- The canvas size (pixels) of a genome's sprite, if it has one (body/<sprite>.lua pixelW/pixelH).
function E.genomeSize(game, genome)
  local ok, g = pcall(dofile, L.genome(game) .. genome .. ".lua")
  if not ok or type(g) ~= "table" then
    local env = setmetatable({ require = function() return setmetatable({}, { __index = function() return function() end end }) end }, { __index = _G })
    local f = loadfile(L.genome(game) .. genome .. ".lua", "t", env)
    ok, g = pcall(f or function() end)
    if not ok or type(g) ~= "table" then return nil end
  end
  local spr = g.sprite2d or g.sprite
  if type(spr) ~= "string" then return nil end
  local ok2, body = pcall(dofile, L.genome(game) .. L.layout.BODY .. spr .. ".lua")
  if not ok2 or type(body) ~= "table" then return nil end
  for _, layer in ipairs(body.layers or {}) do
    if layer.pixelW and layer.pixelH then return layer.pixelW, layer.pixelH end
  end
  return nil
end

-- Draws the outline markers of every entity slice on the entities layer (frame 1), replacing the old ones.
function E.drawMarkers(sprite)
  local layer = E.layer(sprite, true)
  local cel = layer:cel(1)
  if cel then sprite:deleteCel(cel) end
  local list = E.list(sprite)
  if #list == 0 then return end
  local img = Image(sprite.width, sprite.height, sprite.colorMode)
  local color = sprite.colorMode == ColorMode.INDEXED and 1 or app.pixelColor.rgba(255, 0, 255, 255)
  for _, e in ipairs(list) do
    local b = e.slice.bounds
    for x = b.x, b.x + b.width - 1 do
      img:drawPixel(x, b.y, color)
      img:drawPixel(x, b.y + b.height - 1, color)
    end
    for y = b.y, b.y + b.height - 1 do
      img:drawPixel(b.x, y, color)
      img:drawPixel(b.x + b.width - 1, y, color)
    end
  end
  sprite:newCel(layer, 1, img, Point(0, 0))
end

-- Adds an entity slice at x, y (cells; w/h in cells, default the genome's sprite size or 4x4 px).
function E.add(sprite, game, genome, x, y, opts)
  opts = opts or {}
  local pw, ph = E.genomeSize(game, genome)
  local wpx = opts.w and math.max(1, math.floor(opts.w / 2)) or pw or 4
  local hpx = opts.h or ph or 4
  local s = sprite:newSlice(Rectangle(math.floor(x / 2), math.floor(y), wpx, hpx))
  s.name = nextName(sprite, genome)
  s.color = Color { r = 255, g = 0, b = 255, a = 255 }
  local order = 0
  for _, e in ipairs(E.list(sprite)) do order = math.max(order, e.order) end
  s.properties = { genome = genome, fg = opts.fg or 1, extra = opts.extra or "", order = order + 1 }
  return s
end

-- ---- the scene file side ----
-- Replaces (or adds, or removes) the generated block of every fg layer. Returns the number of entities written.
function E.write(sprite, game, scene, spriteName)
  local file = L.sceneFile(game, scene)
  local src = L.readAll(file)
  local var = src:match("\nreturn%s+([%a_][%w_]*)%s*$")
  assert(var, file .. ": doesn't end in 'return <table>'")
  local byLayer, order = {}, {}
  for _, e in ipairs(E.list(sprite)) do
    if not byLayer[e.fg] then byLayer[e.fg] = {}; order[#order + 1] = e.fg end
    local extra = e.extra
    if e.genome == "door" then  -- a door's size is its slice's
      local f = E.parseExtra(extra)
      if type(f.door) == "table" then
        f.door.w, f.door.h = e.w, e.h
        extra = E.extraText(f)
      end
    end
    table.insert(byLayer[e.fg], E.entryText(e.genome, e.x, e.y, extra))
  end
  local beginLine = string.format(E.BEGIN, spriteName)
  -- Drop every old block first.
  src = src:gsub("\n[ \t]*%-%- BEGIN entities %(generated[^\n]*\n.-[ \t]*%-%- END entities[^\n]*", "")
  local fgStart = src:find(var .. "%.fg%s*=%s*{")
  table.sort(order)
  for _, layer in ipairs(order) do
    local s, e
    if fgStart then s, e = src:find("%[%s*" .. layer .. "%s*%]%s*=%s*{[ \t]*", fgStart) end
    local lines = byLayer[layer]
    if s then
      local block = "\n    " .. beginLine .. "\n    " .. table.concat(lines, "\n    ") .. "\n    " .. E.END
      src = src:sub(1, e) .. block .. src:sub(e + 1)
    elseif fgStart then
      local close = select(2, src:find(var .. "%.fg%s*=%s*{", fgStart))
      local block = "\n  [" .. layer .. "] = {\n    " .. beginLine .. "\n    " .. table.concat(lines, "\n    ") ..
                    "\n    " .. E.END .. "\n  },"
      src = src:sub(1, close) .. block .. src:sub(close + 1)
    else
      local rs = src:find("\nreturn%s+[%a_][%w_]*%s*$")
      src = src:sub(1, rs) .. var .. ".fg = {\n  [" .. layer .. "] = {\n    " .. beginLine .. "\n    " ..
            table.concat(lines, "\n    ") .. "\n    " .. E.END .. "\n  }\n}\n" .. src:sub(rs + 1)
      fgStart = src:find(var .. "%.fg%s*=%s*{")
    end
  end
  L.writeAll(file, src)
  local n = 0
  for _, l in pairs(byLayer) do n = n + #l end
  return n
end

-- The scene a sprite places entities into: its file name's scene (bg = "<sprite>"), in the game of its path.
function E.sceneOf(sprite)
  local name = sprite.filename:gsub("\\", "/"):match("([^/]+)%.[^.]+$")
  local game = L.gameOf(sprite.filename)
  return game, game and L.sceneOfSprite(game, name), name
end

-- After adding/moving entities: markers + scene file. Quietly does nothing for sprites that aren't a scene's bg.
function E.sync(sprite)
  if #E.list(sprite) == 0 and not E.layer(sprite) then return 0 end
  local game, scene, name = E.sceneOf(sprite)
  if not scene then return 0 end
  E.drawMarkers(sprite)
  return E.write(sprite, game, scene, name)
end

-- Moves a scene file's hand-written fg entries onto the sprite as slices (one-time migration; keeps the order).
-- Entries that aren't plain data stay in the file.
function E.import(sprite, game, scene)
  local tbl = assert(L.loadScene(game, scene))
  local file = L.sceneFile(game, scene)
  local src = L.readAll(file)
  local layers = {}
  for k in pairs(tbl.fg or {}) do layers[#layers + 1] = k end
  table.sort(layers)
  local n = 0
  for _, layer in ipairs(layers) do
    for _, entry in ipairs(tbl.fg[layer]) do
      if type(entry) == "table" and entry.genome and type(entry.pos) == "table" then
        local extra = E.extraText(entry)
        local opts = { fg = layer, extra = extra }
        if entry.genome == "door" and type(entry.door) == "table" then opts.w, opts.h = entry.door.w, entry.door.h end
        E.add(sprite, game, entry.genome, entry.pos[1], entry.pos[2], opts)
        n = n + 1
      end
    end
  end
  -- Take the imported entries out of the file: every line that is one '{ genome = ... },' entry.
  local out = {}
  for line in (src .. "\n"):gmatch("([^\n]*)\n") do
    local entryText = line:match("^%s*({%s*genome%s*=.*})%s*,?%s*%-?%-?.*$")
    local keep = true
    if entryText then
      local f = load("return " .. entryText, "=entry", "t", {})
      if f and pcall(f) then keep = false end
    end
    if keep then out[#out + 1] = line end
  end
  while out[#out] == "" do out[#out] = nil end
  L.writeAll(file, table.concat(out, "\n") .. "\n")
  return n
end

return E
