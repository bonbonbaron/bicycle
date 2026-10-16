-- Regenerates gen/ids.lua and gen/ids.h (tools/ids/genids.lua, README "IDs") from inside Aseprite, so a tag, scene,
-- spawn or song you just exported is known to the engine without a rebuild. sprite.lua, adddoor.lua and addsong.lua
-- call this after writing; the build reruns the same generator anyway. Never edits ids by hand; never fails an
-- export (problems are printed).
local current_file = debug.getinfo(1, "S").source:match("^@?(.*)")
local current_dir = current_file:match("^(.*[/\\])") or "./"
local root = os.getenv("BICYCLE_REPO") or (current_dir .. "../../")

local function isDir(p)
  if app and app.fs then return app.fs.isDirectory(p) end
  local ok = os.execute('test -d "' .. p .. '"')
  return ok == true or ok == 0
end
local function list(dir)
  if app and app.fs then return app.fs.listFiles(dir) end
  local out, p = {}, io.popen('ls -1 "' .. dir .. '" 2>/dev/null')
  if p then for l in p:lines() do out[#out + 1] = l end p:close() end
  return out
end
local function walk(dir, out)
  for _, f in ipairs(list(dir)) do
    local p = dir .. "/" .. f
    if isDir(p) then walk(p, out) elseif f ~= ".gitkeep" then out[#out + 1] = p end
  end
end

local function refresh()
  local gen = root .. "tools/ids/genids.lua"
  if not io.open(gen, "r") then return end  -- not in a bicycle checkout
  local files = { root .. "include/v/common/ColorPalette.h" }
  for _, d in ipairs(list(root)) do
    local g = root .. d
    if isDir(g .. "/genome") then
      walk(g .. "/genome", files)
      files[#files + 1] = g .. "/main.lua"
    end
  end
  for _, f in ipairs(list(root .. "scripts")) do
    if f:match("%.lua$") then files[#files + 1] = root .. "scripts/" .. f end
  end
  if app and app.fs and app.fs.makeAllDirectories then app.fs.makeAllDirectories(root .. "gen") end
  local tmp = root .. "gen/ids-inputs-aseprite.txt"  -- gen/ is generated and ignored by git
  local fh = io.open(tmp, "w")
  if not fh then return end
  fh:write(table.concat(files, "\n"), "\n")
  fh:close()
  local chunk, err = loadfile(gen)
  if not chunk then print("refreshids: " .. tostring(err)) return end
  chunk("--root", root, "--out", root .. "gen", "--list", tmp)
end

return function()
  local ok, e = pcall(refresh)
  if not ok then print("refreshids: " .. tostring(e)) end
end
