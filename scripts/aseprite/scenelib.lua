-- Shared by adddoor.lua and addsong.lua: find a game's scenes, their spawns and doors, its songs and SoundFonts,
-- and edit a scene file (genome/scene/<name>.lua) in place. Plain Lua plus app.fs when it's there, so it also
-- runs outside Aseprite (tests).
local current_file = debug.getinfo(1, "S").source:match("^@?(.*)")
local current_dir = current_file:match("^(.*[/\\])") or "./"
local Layout = dofile(current_dir .. "../GenomeLayout.lua")

local L = { layout = Layout }
L.repo = os.getenv("BICYCLE_REPO") or (current_dir .. "../../")

local function norm(p) return (p:gsub("\\", "/")) end
local function exists(p)
  local f = io.open(p, "r")
  if f then f:close() return true end
  return false
end
local function readAll(p)
  local f = assert(io.open(p, "rb"), "can't read " .. p)
  local s = f:read("a")
  f:close()
  return s
end
local function writeAll(p, s)
  local f = assert(io.open(p, "wb"), "can't write " .. p)
  f:write(s)
  f:close()
end
L.readAll, L.writeAll = readAll, writeAll

-- Files (not directories) in a directory, sorted.
function L.listFiles(dir)
  local out = {}
  if app and app.fs and app.fs.listFiles then
    for _, f in ipairs(app.fs.listFiles(dir)) do
      if not app.fs.isDirectory(app.fs.joinPath(dir, f)) then out[#out + 1] = f end
    end
  else
    local p = io.popen('ls -1p "' .. dir .. '" 2>/dev/null')
    if p then
      for line in p:lines() do
        if not line:find("/$") then out[#out + 1] = line end
      end
      p:close()
    end
  end
  table.sort(out)
  return out
end

-- Every game directory in the repo (a directory with genome/scene/).
function L.games()
  local out = {}
  local p = io.popen('ls -1d "' .. L.repo .. '"*/genome/scene 2>/dev/null')
  if p then
    for line in p:lines() do out[#out + 1] = norm(line):match("([^/]+)/genome/scene$") end
    p:close()
  end
  table.sort(out)
  return out
end

function L.genome(game) return L.repo .. game .. "/genome/" end
function L.sceneFile(game, scene) return L.genome(game) .. Layout.SCENE .. scene .. ".lua" end

-- The game a file belongs to: the path component right before aseprite/ or genome/ (e.g. test/aseprite/x.aseprite
-- -> "test"), else the first of L.games() whose name appears in the path.
function L.gameOf(path)
  path = norm(path or "")
  local g = path:match("([^/]+)/aseprite/[^/]*$") or path:match("([^/]+)/genome/")
  if g and exists(L.genome(g) .. Layout.SCENE) then return g end
  for _, game in ipairs(L.games()) do
    if path:find("/" .. game .. "/", 1, true) then return game end
  end
  return nil
end

-- Loads a scene table without the engine: unknown globals and require() give harmless stand-ins.
local function stub()
  local t = {}
  return setmetatable(t, { __index = function() return stub() end, __call = function() return stub() end })
end
function L.loadScene(game, scene)
  local file = L.sceneFile(game, scene)
  local env = setmetatable({ require = function() return stub() end }, { __index = function(_, k)
    return _G[k] or stub()
  end })
  local chunk, err = load(readAll(file), "@" .. file, "t", env)
  if not chunk then return nil, err end
  local ok, tbl = pcall(chunk)
  if not ok or type(tbl) ~= "table" then return nil, tostring(tbl) end
  return tbl
end

-- Scene names (file stems in genome/scene/).
function L.scenes(game)
  local out = {}
  for _, f in ipairs(L.listFiles(L.genome(game) .. Layout.SCENE)) do
    local n = f:match("^(.+)%.lua$")
    if n then out[#out + 1] = n end
  end
  return out
end

local function sortedKeys(t)
  local out = {}
  for k in pairs(t or {}) do if type(k) == "string" then out[#out + 1] = k end end
  table.sort(out)
  return out
end

-- { spawns = { names }, doors = { { to, spawn, pos, w, h, on } }, bg, music } of a scene.
function L.sceneInfo(game, scene)
  local tbl, err = L.loadScene(game, scene)
  if not tbl then return { spawns = {}, doors = {}, error = err } end
  local info = { spawns = sortedKeys(tbl.spawns), doors = {}, bg = tbl.bg, music = tbl.music, fgLayers = {} }
  for layer, list in pairs(tbl.fg or {}) do
    info.fgLayers[#info.fgLayers + 1] = layer
    for _, e in ipairs(list) do
      if type(e) == "table" and type(e.door) == "table" then
        local d = e.door
        info.doors[#info.doors + 1] = { to = d.to, spawn = d.spawn, pos = e.pos, w = d.w, h = d.h, on = d.on }
      end
    end
  end
  table.sort(info.fgLayers)
  return info
end

-- The spawn (and door) choices offered for doors into a scene: its spawns, plus "default" if it has none.
function L.spawnChoices(game, scene)
  local s = L.sceneInfo(game, scene).spawns
  if #s == 0 then s = { "default" } end
  return s
end

-- The scene whose bg is this sprite (a background exported from it), if any.
function L.sceneOfSprite(game, spriteName)
  for _, s in ipairs(L.scenes(game)) do
    local info = L.sceneInfo(game, s)
    if info.bg == spriteName then return s end
  end
  return nil
end

-- The bare name of an asset reference: "meadow" for "meadow" or an old path like "music/meadow.sf2".
function L.bare(v)
  if type(v) ~= "string" then return v end
  return (v:gsub("\\", "/"):match("([^/]*)$"):gsub("%.[^.]*$", ""))
end

-- Songs (.mid) and SoundFonts (.sf2) of a game, as the bare names scenes and genomes use ("meadow": the folder
-- and extension come from the field, scripts/GenomeLayout.lua).
function L.songs(game)
  local mids, sf2s = {}, {}
  for _, f in ipairs(L.listFiles(L.genome(game) .. Layout.MUSIC)) do
    local low = f:lower()
    if low:find("%.midi?$") then mids[#mids + 1] = L.bare(f) end
    if low:find("%.sf2$") then sf2s[#sf2s + 1] = L.bare(f) end
  end
  table.sort(mids)
  table.sort(sf2s)
  return mids, sf2s
end

-- ---- editing a scene file ----
-- Scene files are hand-written Lua, so edits are small text insertions that keep everything else as it is.

-- The local the scene table is returned as ("return intro" -> "intro").
local function sceneVar(src)
  return src:match("\nreturn%s+([%a_][%w_]*)%s*$") or src:match("^return%s+([%a_][%w_]*)") or
         src:match("return%s+([%a_][%w_]*)%s*$")
end

-- Inserts text right before the final "return <var>".
local function beforeReturn(src, text)
  local s, e = src:find("\nreturn%s+[%a_][%w_]*%s*$")
  assert(s, "scene file doesn't end in 'return <table>'")
  return src:sub(1, s) .. text .. src:sub(s + 1)
end

local function fmtNum(n)
  if math.type and math.type(n) == "integer" then return tostring(n) end
  if n == math.floor(n) then return string.format("%d", n) end
  return tostring(n)
end

-- Adds a door entity to scene.fg[layer] (the first fg layer by default):
--   { genome = "door", pos = { x, y }, door = { to = ..., spawn = ..., w = ..., h = ..., on = ... } }
function L.addDoor(game, scene, d)
  local file = L.sceneFile(game, scene)
  local src = readAll(file)
  local var = assert(sceneVar(src), file .. ": can't find the scene table's name")
  local info = L.sceneInfo(game, scene)
  local layer = d.layer or info.fgLayers[1] or 1
  local fields = { 'to = "' .. d.to .. '"' }
  if d.spawn and d.spawn ~= "" then fields[#fields + 1] = 'spawn = "' .. d.spawn .. '"' end
  fields[#fields + 1] = "w = " .. fmtNum(d.w)
  fields[#fields + 1] = "h = " .. fmtNum(d.h)
  if d.on and d.on ~= "touch" then fields[#fields + 1] = 'on = "' .. d.on .. '"' end
  if d.transition and d.transition ~= "" then fields[#fields + 1] = 'transition = "' .. d.transition .. '"' end
  local entry = '{ genome = "door", pos = { ' .. fmtNum(d.x) .. ', ' .. fmtNum(d.y) .. ' }, door = { ' ..
                table.concat(fields, ", ") .. ' } },'
  -- Find "[layer] = {" inside the fg table and add the entry as its first item.
  local fgStart = src:find(var .. "%.fg%s*=%s*{")
  local s, e
  if fgStart then
    s, e = src:find("%[%s*" .. layer .. "%s*%]%s*=%s*{[ \t]*\n?", fgStart)
  end
  if s then
    local indent = src:sub(e + 1):match("^([ \t]*)") or "    "
    if indent == "" then indent = "    " end
    src = src:sub(1, e) .. indent .. "-- added by scripts/aseprite/adddoor.lua\n" .. indent .. entry .. "\n" ..
          src:sub(e + 1)
  elseif fgStart then
    error(file .. ": no fg layer [" .. layer .. "] to put the door on")
  else
    src = beforeReturn(src, var .. ".fg = {\n  [" .. layer .. "] = {\n    " .. entry .. "\n  }\n}\n")
  end
  writeAll(file, src)
  return entry
end

-- Adds a named spawn { x, y } (no-op if the scene already has it). Returns true if it added one.
function L.addSpawn(game, scene, name, x, y)
  local info = L.sceneInfo(game, scene)
  for _, s in ipairs(info.spawns) do if s == name then return false end end
  local file = L.sceneFile(game, scene)
  local src = readAll(file)
  local var = assert(sceneVar(src), file .. ": can't find the scene table's name")
  local item = name .. " = { " .. fmtNum(x) .. ", " .. fmtNum(y) .. " }"
  local s, e = src:find(var .. "%.spawns%s*=%s*{%s*")
  if s then
    local rest = src:sub(e + 1)
    local sep = rest:match("^}") and " " or ", "
    src = src:sub(1, e) .. item .. sep .. rest
  else
    src = beforeReturn(src, var .. ".spawns = { " .. item .. " }  -- added by scripts/aseprite/adddoor.lua\n")
  end
  writeAll(file, src)
  return true
end

-- Sets scene.music = { soundfont = ..., song = ..., transition = ... } (replacing a one-line music setting).
function L.setMusic(game, scene, m)
  local file = L.sceneFile(game, scene)
  local src = readAll(file)
  local var = assert(sceneVar(src), file .. ": can't find the scene table's name")
  local fields = {}
  if m.soundfont and m.soundfont ~= "" then fields[#fields + 1] = 'soundfont = "' .. L.bare(m.soundfont) .. '"' end
  if m.song and m.song ~= "" then fields[#fields + 1] = 'song = "' .. L.bare(m.song) .. '"' end
  if m.transition and m.transition ~= "" and m.transition ~= "crossfade" then
    fields[#fields + 1] = 'transition = "' .. m.transition .. '"'
  end
  local line = var .. ".music = { " .. table.concat(fields, ", ") .. " }"
  local pat = "\n" .. var .. "%.music%s*=%s*%b{}[^\n]*"
  if src:find(pat) then
    src = src:gsub(pat, function() return "\n" .. line end, 1)
  elseif src:find("^" .. var .. "%.music%s*=") then
    src = src:gsub("^" .. var .. "%.music%s*=%s*%b{}[^\n]*", line, 1)
  else
    src = beforeReturn(src, line .. "\n")
  end
  writeAll(file, src)
  return line
end

return L
