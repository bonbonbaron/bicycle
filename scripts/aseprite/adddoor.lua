-- Adds a door to a scene (File > Scripts > adddoor): a door gene entry in genome/scene/<scene>.lua that sends the
-- hero to another scene's spawn (scripts/Genome.lua, door gene; scripts/Scene.lua, Scene.go).
--
-- GUI: open the scene's background sprite (e.g. test/aseprite/meadow.aseprite), select the doorway with the
-- rectangle marquee (or add a slice named "door"), run the script, pick the target scene and spawn, OK.
-- Positions are in cells: one sprite pixel is 2 cells wide and 1 tall (README.txt, BACKGROUNDS).
--
-- Batch / CLI (no sprite needed; x y w h in cells):
--   aseprite -b --script-param game=test --script-param scene=intro --script-param to=house \
--     --script-param spawn=entry --script-param x=64 --script-param y=40 --script-param w=6 --script-param h=4 \
--     [--script-param on=press] [--script-param return_spawn=from_house] [--script-param layer=4] \
--     --script scripts/aseprite/adddoor.lua
--   aseprite -b --script-param game=test --script-param list=1 [--script-param to=house] --script scripts/aseprite/adddoor.lua
--     prints the choices the drop-downs offer: "scene <name>" lines, and "spawn <scene> <name>" lines.
-- A sprite given on the command line before --script also works: game/scene default from its path, and the
-- door from its selection or "door" slice.
local current_file = debug.getinfo(1, "S").source:match("^@?(.*)")
local current_dir = current_file:match("^(.*[/\\])") or "./"
package.path = current_dir .. "?.lua;" .. package.path
local L = require("scenelib")

local params = (app and app.params) or {}
local sprite = app and app.sprite or nil

-- Where the door goes on the canvas, in cells: a "door" slice, else the selection.
local function canvasRect()
  if not sprite then return nil end
  for _, slice in ipairs(sprite.slices) do
    if slice.name:lower():find("door") then
      local b = slice.bounds
      return { x = b.x * 2, y = b.y, w = b.width * 2, h = b.height }
    end
  end
  local sel = sprite.selection
  if sel and not sel.isEmpty then
    local b = sel.bounds
    return { x = b.x * 2, y = b.y, w = b.width * 2, h = b.height }
  end
  return nil
end

local function spriteName()
  if not sprite or not sprite.filename then return nil end
  return sprite.filename:gsub("\\", "/"):match("([^/]+)%.[^.]+$")
end

-- The return spawn: just left of the door, so arriving through the target's door back lands beside it.
local function returnSpawnPos(d)
  return math.max(0, d.x - 10), math.max(0, d.y - 2)
end

local E = require("sceneentities")

local function apply(game, scene, d, returnName)
  local sg, ss = nil, nil
  if sprite then sg, ss = E.sceneOf(sprite) end
  if sprite and sg == game and ss == scene then
    -- The scene's own background is open: the door is a slice on its entities layer (README, "Placing entities").
    local f = { to = d.to }
    if d.spawn and d.spawn ~= "" then f.spawn = d.spawn end
    if d.on and d.on ~= "touch" then f.on = d.on end
    if d.transition and d.transition ~= "" then f.transition = d.transition end
    local fg = d.layer or L.sceneInfo(game, scene).fgLayers[1] or 1
    local slice = E.add(sprite, game, "door", d.x, d.y, { w = d.w, h = d.h, fg = fg, extra = "door = " .. E.ser(f) })
    E.sync(sprite)
    if not (app and app.isUIAvailable) then sprite:saveAs(sprite.filename) end
    print("adddoor: " .. slice.name .. " on " .. E.LAYER .. "; " .. L.sceneFile(game, scene) .. " rewritten")
  else
    local entry = L.addDoor(game, scene, d)
    print("adddoor: " .. L.sceneFile(game, scene) .. ": " .. entry)
  end
  if returnName and returnName ~= "" then
    local x, y = returnSpawnPos(d)
    if L.addSpawn(game, scene, returnName, x, y) then
      print("adddoor: added spawn " .. returnName .. " = { " .. x .. ", " .. y .. " } to " .. scene)
    else
      print("adddoor: " .. scene .. " already has spawn " .. returnName)
    end
    -- The target scene's doors back here should use it: tell the user if none does yet.
    local back = false
    for _, td in ipairs(L.sceneInfo(game, d.to).doors) do
      if td.to == scene then back = true end
    end
    if not back then
      print("adddoor: note: " .. d.to .. " has no door back to " .. scene .. " yet (use spawn=" .. returnName .. ")")
    end
  end
end

local function listChoices(game, to)
  for _, s in ipairs(L.scenes(game)) do print("scene " .. s) end
  local targets = to and { to } or L.scenes(game)
  for _, s in ipairs(targets) do
    for _, sp in ipairs(L.spawnChoices(game, s)) do print("spawn " .. s .. " " .. sp) end
  end
end

-- ---- batch ----
local isBatch = not (app and app.isUIAvailable)
if params.list or params.to or isBatch then
  local game = params.game or L.gameOf(sprite and sprite.filename) or "test"
  if params.list then
    listChoices(game, params.to)
    return
  end
  local scene = params.scene or L.sceneOfSprite(game, spriteName() or "")
  assert(scene, "adddoor: which scene? (--script-param scene=<name>)")
  assert(params.to, "adddoor: where to? (--script-param to=<scene>)")
  local rect = canvasRect() or {}
  local d = {
    to = params.to,
    spawn = params.spawn or L.spawnChoices(game, params.to)[1],
    x = tonumber(params.x) or rect.x, y = tonumber(params.y) or rect.y,
    w = tonumber(params.w) or rect.w or 4, h = tonumber(params.h) or rect.h or 4,
    on = params.on or "touch", layer = tonumber(params.layer), transition = params.transition,
  }
  assert(d.x and d.y, "adddoor: where? (x= y= in cells, or a selection / \"door\" slice on the sprite)")
  local ok = false
  for _, s in ipairs(L.scenes(game)) do if s == d.to then ok = true end end
  assert(ok, "adddoor: " .. game .. " has no scene '" .. d.to .. "'")
  apply(game, scene, d, params.return_spawn)
  require("refreshids")()  -- gen/ids.lua knows the new spawn/door now (README, "IDs")
  return
end

-- ---- GUI ----
local game = L.gameOf(sprite and sprite.filename) or L.games()[1] or "test"
local scene = L.sceneOfSprite(game, spriteName() or "") or L.scenes(game)[1]
local scenes = L.scenes(game)
local rect = canvasRect() or { x = 0, y = 0, w = 4, h = 4 }
local to = scenes[1]
for _, s in ipairs(scenes) do if s ~= scene then to = s break end end

local dlg = Dialog("Add door")
local function refreshSpawns()
  local t = dlg.data.to
  local choices = t and L.spawnChoices(dlg.data.game, t) or {}
  dlg:modify { id = "spawn", options = choices, option = choices[1] }
  dlg:modify { id = "return_name", text = "from_" .. (t or "") }
end
local function refreshScenes()
  local list = L.scenes(dlg.data.game)
  dlg:modify { id = "scene", options = list, option = list[1] }
  dlg:modify { id = "to", options = list, option = list[1] }
  refreshSpawns()
end
dlg:combobox { id = "game", label = "Game", option = game, options = L.games(), onchange = refreshScenes }
dlg:combobox { id = "scene", label = "This scene", option = scene, options = scenes }
dlg:separator { text = "Goes to" }
dlg:combobox { id = "to", label = "Scene", option = to, options = scenes, onchange = refreshSpawns }
dlg:combobox { id = "spawn", label = "Spawn", options = to and L.spawnChoices(game, to) or {} }
dlg:separator { text = "Door (cells; from the selection or a \"door\" slice)" }
dlg:number { id = "x", label = "x", text = tostring(rect.x), decimals = 0 }
dlg:number { id = "y", label = "y", text = tostring(rect.y), decimals = 0 }
dlg:number { id = "w", label = "w", text = tostring(rect.w), decimals = 0 }
dlg:number { id = "h", label = "h", text = tostring(rect.h), decimals = 0 }
dlg:combobox { id = "on", label = "Opens on", option = "touch", options = { "touch", "press" } }
dlg:combobox { id = "transition", label = "Transition", option = "fade", options = { "fade", "wipe", "dissolve", "none" } }
dlg:separator { text = "Coming back" }
dlg:check { id = "return_spawn", text = "Add a return spawn here if missing", selected = true }
dlg:entry { id = "return_name", label = "Spawn name", text = "from_" .. (to or "") }
dlg:button { id = "ok", text = "&OK", focus = true }
dlg:button { id = "cancel", text = "&Cancel" }
dlg:show()

local data = dlg.data
if not data.ok then return end
if not data.scene or not data.to then
  app.alert("Pick this scene and the scene the door goes to.")
  return
end
local d = { to = data.to, spawn = data.spawn, x = data.x, y = data.y, w = data.w, h = data.h, on = data.on,
            transition = data.transition ~= "fade" and data.transition or nil }
apply(data.game, data.scene, d, data.return_spawn and data.return_name or nil)
app.alert("Door to " .. data.to .. " (" .. (data.spawn or "default") .. ") added to " .. data.scene .. ".")
require("refreshids")()
