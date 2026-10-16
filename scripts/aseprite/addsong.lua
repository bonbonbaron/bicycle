-- Sets a scene's music (File > Scripts > addsong): writes scene.music = { soundfont = ..., song = ...,
-- transition = ... } in genome/scene/<scene>.lua (scripts/Sound.lua plays it; scripts/Scene.lua uses transition
-- as the default when the scene is entered: crossfade (default), cut or keep).
--
-- GUI: open any sprite of the game (or none), run the script, pick the scene, song and SoundFont, OK.
-- Batch / CLI:
--   aseprite -b --script-param game=test --script-param scene=house --script-param song=music/house.mid \
--     [--script-param soundfont=music/meadow.sf2] [--script-param transition=cut] --script scripts/aseprite/addsong.lua
--   aseprite -b --script-param game=test --script-param list=1 --script scripts/aseprite/addsong.lua
--     prints the drop-downs' choices: "scene <name>", "song <path>", "soundfont <path>" lines.
local current_file = debug.getinfo(1, "S").source:match("^@?(.*)")
local current_dir = current_file:match("^(.*[/\\])") or "./"
package.path = current_dir .. "?.lua;" .. package.path
local L = require("scenelib")

local params = (app and app.params) or {}
local sprite = app and app.sprite or nil
local TRANSITIONS = { "crossfade", "cut", "keep" }

local function spriteName()
  if not sprite or not sprite.filename then return nil end
  return sprite.filename:gsub("\\", "/"):match("([^/]+)%.[^.]+$")
end

local function contains(list, v)
  for _, x in ipairs(list) do if x == v then return true end end
  return false
end

local isBatch = not (app and app.isUIAvailable)
if params.list or params.song or isBatch then
  local game = params.game or L.gameOf(sprite and sprite.filename) or "test"
  local mids, sf2s = L.songs(game)
  if params.list then
    for _, s in ipairs(L.scenes(game)) do print("scene " .. s) end
    for _, s in ipairs(mids) do print("song " .. s) end
    for _, s in ipairs(sf2s) do print("soundfont " .. s) end
    return
  end
  local scene = params.scene or L.sceneOfSprite(game, spriteName() or "")
  assert(scene and contains(L.scenes(game), scene), "addsong: which scene? (--script-param scene=<name>)")
  assert(params.song and contains(mids, params.song), "addsong: song must be one of: " .. table.concat(mids, ", "))
  local current = L.sceneInfo(game, scene).music or {}
  local sf = params.soundfont or current.soundfont or sf2s[1]
  assert(sf == nil or contains(sf2s, sf), "addsong: soundfont must be one of: " .. table.concat(sf2s, ", "))
  local tr = params.transition or "crossfade"
  assert(contains(TRANSITIONS, tr), "addsong: transition must be crossfade, cut or keep")
  print("addsong: " .. L.sceneFile(game, scene) .. ": " ..
        L.setMusic(game, scene, { song = params.song, soundfont = sf, transition = tr }))
  return
end

-- ---- GUI ----
local game = L.gameOf(sprite and sprite.filename) or L.games()[1] or "test"
local scenes = L.scenes(game)
local scene = L.sceneOfSprite(game, spriteName() or "") or scenes[1]
local mids, sf2s = L.songs(game)
local cur = scene and L.sceneInfo(game, scene).music or {}

local dlg = Dialog("Scene music")
local function refresh()
  local list = L.scenes(dlg.data.game)
  local m, s = L.songs(dlg.data.game)
  dlg:modify { id = "scene", options = list, option = list[1] }
  dlg:modify { id = "song", options = m, option = m[1] }
  dlg:modify { id = "soundfont", options = s, option = s[1] }
end
local function sceneChanged()
  local m = L.sceneInfo(dlg.data.game, dlg.data.scene).music or {}
  if m.song then dlg:modify { id = "song", option = m.song } end
  if m.soundfont then dlg:modify { id = "soundfont", option = m.soundfont } end
  dlg:modify { id = "transition", option = m.transition or "crossfade" }
end
dlg:combobox { id = "game", label = "Game", option = game, options = L.games(), onchange = refresh }
dlg:combobox { id = "scene", label = "Scene", option = scene, options = scenes, onchange = sceneChanged }
dlg:combobox { id = "song", label = "Song", option = cur.song or mids[1], options = mids }
dlg:combobox { id = "soundfont", label = "SoundFont", option = cur.soundfont or sf2s[1], options = sf2s }
dlg:combobox { id = "transition", label = "On entering", option = cur.transition or "crossfade", options = TRANSITIONS }
dlg:button { id = "ok", text = "&OK", focus = true }
dlg:button { id = "cancel", text = "&Cancel" }
dlg:show()

local data = dlg.data
if not data.ok then return end
if not data.scene or not data.song then
  app.alert("Pick a scene and a song (MIDI files go in <game>/genome/music/).")
  return
end
local line = L.setMusic(data.game, data.scene, { song = data.song, soundfont = data.soundfont, transition = data.transition })
app.alert(data.scene .. ": " .. line)
