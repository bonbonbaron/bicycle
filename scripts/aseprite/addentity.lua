-- Places an entity in a scene (File > Scripts > addentity): a slice on the scene's background sprite, marked on its
-- "entities" layer, then written into genome/scene/<scene>.lua (sceneentities.lua; README, "Placing entities in
-- Aseprite"). Move or resize the slices later and run it again (or sprite.lua) to rewrite the scene's entities.
--
-- GUI: open the scene's background (e.g. test/aseprite/meadow.aseprite), select where it goes with the marquee,
-- run the script, pick the genome (the game's genome/*.lua, the same names as gen/ids.lua's ids.genome), the fg
-- layer and options, OK. Doors: scripts/aseprite/adddoor.lua (they're slices on the same layer).
-- Batch / CLI (with the sprite on the command line; x y in cells; the sprite is saved):
--   aseprite -b test/aseprite/meadow.aseprite --script-param genome=coin --script-param x=30 --script-param y=42 \
--     [--script-param fg=4] [--script-param persist=1] [--script-param focus=1] [--script-param motion=spin] \
--     [--script-param extra='key = "gem"'] --script scripts/aseprite/addentity.lua
--   ... --script-param list=1        prints "genome <name>" (the drop-down) and "entity <slice> <genome> x y" lines
--   ... --script-param sync=1        rewrites the scene file from the slices (no new entity)
--   ... --script-param import=1      one-time: moves the scene file's hand-written entries onto slices
local current_file = debug.getinfo(1, "S").source:match("^@?(.*)")
local current_dir = current_file:match("^(.*[/\\])") or "./"
package.path = current_dir .. "?.lua;" .. package.path
local L = require("scenelib")
local E = require("sceneentities")

local params = (app and app.params) or {}
local sprite = app and app.sprite or nil

-- Motion presets (the motion gene, scripts/Genome.lua): a starting point to tweak in the scene file or extra.
local MOTIONS = {
  none = nil,
  linear = 'motion = { type = "linear", speed = 6, path = { { 0, 0 }, { 20, 0 } } }',
  orbital = 'motion = { type = "orbital", radius = 6, period = 3 }',
  spin = 'motion = { type = "spin", period = 2 }',
}
local MOTION_NAMES = { "none", "linear", "orbital", "spin" }

-- Genome names of a game (genome/*.lua), the same set as gen/ids.lua's ids.genome.
local function genomes(game)
  local out = {}
  for _, f in ipairs(L.listFiles(L.genome(game))) do
    local n = f:match("^(.+)%.lua$")
    -- (a starter genome that only names a scene, like test's intro.lua, isn't something to place)
    local src = n and L.readAll(L.genome(game) .. f) or ""
    if n and not src:match("\n%s*scene%s*=%s*\"") then out[#out + 1] = n end
  end
  return out
end

local function extraFor(o)
  local parts = {}
  if o.focus then parts[#parts + 1] = "focus = true" end
  if o.persist then parts[#parts + 1] = "persist = true" end
  if o.motion and MOTIONS[o.motion] then parts[#parts + 1] = MOTIONS[o.motion] end
  if o.extra and o.extra ~= "" then parts[#parts + 1] = o.extra end
  local text = table.concat(parts, ", ")
  return E.extraText(E.parseExtra(text))  -- validates and normalizes
end

local function save()
  if not (app and app.isUIAvailable) then sprite:saveAs(sprite.filename) end
end

assert(sprite, "addentity: open the scene's background sprite (aseprite -b <sprite>.aseprite ...)")
local game, scene, spriteName = E.sceneOf(sprite)
game = params.game or game
scene = params.scene or scene
assert(game and scene, "addentity: " .. sprite.filename .. " isn't any scene's bg (scene.bg = \"<sprite name>\")")

local isBatch = not (app and app.isUIAvailable)
if isBatch or params.genome or params.list or params.sync or params.import then
  if params.list then
    for _, g in ipairs(genomes(game)) do print("genome " .. g) end
    for _, e in ipairs(E.list(sprite)) do print("entity " .. e.slice.name .. " " .. e.genome .. " " .. e.x .. " " .. e.y) end
    return
  end
  if params.import then
    local n = E.import(sprite, game, scene)
    print("addentity: moved " .. n .. " entries of " .. scene .. " onto " .. spriteName .. "'s entities layer")
  elseif params.genome then
    local ok = false
    for _, g in ipairs(genomes(game)) do if g == params.genome then ok = true end end
    assert(ok, "addentity: " .. game .. " has no genome '" .. params.genome .. "'")
    local rect = (not sprite.selection.isEmpty) and sprite.selection.bounds or nil
    local x = tonumber(params.x) or (rect and rect.x * 2)
    local y = tonumber(params.y) or (rect and rect.y)
    assert(x and y, "addentity: where? (x= y= in cells, or a selection)")
    local s = E.add(sprite, game, params.genome, x, y, { fg = tonumber(params.fg) or L.sceneInfo(game, scene).fgLayers[1] or 1,
      extra = extraFor { focus = params.focus, persist = params.persist, motion = params.motion, extra = params.extra } })
    print("addentity: " .. s.name .. " at " .. x .. "," .. y)
  end
  local n = E.sync(sprite)
  print("addentity: " .. L.sceneFile(game, scene) .. ": " .. n .. " entities")
  save()
  require("refreshids")()
  return
end

-- ---- GUI ----
local list = genomes(game)
local layers = L.sceneInfo(game, scene).fgLayers
local layerNames = {}
for _, l in ipairs(layers) do layerNames[#layerNames + 1] = tostring(l) end
if #layerNames == 0 then layerNames = { "1" } end
local rect = (not sprite.selection.isEmpty) and sprite.selection.bounds or Rectangle(0, 0, 4, 4)

local dlg = Dialog("Add entity to " .. scene)
dlg:combobox { id = "genome", label = "Genome", option = list[1], options = list }
dlg:combobox { id = "fg", label = "Fg layer", option = layerNames[1], options = layerNames }
dlg:number { id = "x", label = "x (cells)", text = tostring(rect.x * 2), decimals = 0 }
dlg:number { id = "y", label = "y (cells)", text = tostring(rect.y), decimals = 0 }
dlg:check { id = "persist", text = "Persists across scenes (persist)" }
dlg:check { id = "focus", text = "Camera follows it (focus)" }
dlg:combobox { id = "motion", label = "Motion", option = "none", options = MOTION_NAMES }
dlg:entry { id = "extra", label = "More fields", text = "" }
dlg:button { id = "ok", text = "&OK", focus = true }
dlg:button { id = "cancel", text = "&Cancel" }
dlg:show()
local d = dlg.data
if not d.ok then return end
app.transaction("Add entity", function()
  E.add(sprite, game, d.genome, d.x, d.y, { fg = tonumber(d.fg),
    extra = extraFor { focus = d.focus, persist = d.persist, motion = d.motion, extra = d.extra } })
  E.sync(sprite)
end)
require("refreshids")()
