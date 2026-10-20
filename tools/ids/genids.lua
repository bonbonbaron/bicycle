-- Build-time string interning (README, "IDs"): scans every game's genome tree, main.lua and the engine scripts and
-- gives each string-keyed runtime name a small integer id, so the engine looks things up by id (bicycle::map,
-- arrays) instead of by string. Generates
--   <out>/ids.h    enum class ids::Anim : uint8_t { ... COUNT } (and Scene, Spawn, ...), plus name tables
--   <out>/ids.lua  { anim = { walk_up = 3, ... }, names = { anim = { [3] = "walk_up", ... } }, ... }
-- and checks references (door targets and spawns, songs, sounds, genomes, initAnim, collision types, literal
-- Scene.go("...") and Sound.play("...")). With --strict (release builds) a bad reference fails the build.
--
-- Runs under LuaJIT's host minilua (Lua 5.1, built with LuaJIT) or any Lua 5.1-5.4: CMake runs it on every build
-- whose inputs changed (CMakeLists.txt, "ids"). Nobody edits an id list by hand: export from Aseprite, add a
-- scene or a song, and the next build regenerates the ids.
--
--   lua tools/ids/genids.lua --root <repo> --out <repo>/gen --list <file listing inputs, one per line>
--       [--strict] [--stamp <file>]
-- LuaJIT's minilua leaves out a few base functions.
tostring = tostring or function( v )
  local t = type( v )
  if t == "string" then return v end
  if t == "number" then return string.format( "%.14g", v ) end
  if t == "nil" then return "nil" end
  if t == "boolean" then return v and "true" or "false" end
  return t
end
print = print or function( s ) io.write( tostring( s ), "\n" ) end

local args = { ... }
local opt = { list = nil, out = nil, root = nil, strict = false, stamp = nil }
do
  local i = 1
  while i <= #args do
    local a = args[i]
    if a == "--strict" then opt.strict = true
    elseif a:sub( 1, 2 ) == "--" then opt[a:sub( 3 )] = args[i + 1]; i = i + 1
    end
    i = i + 1
  end
end
assert( opt.list and opt.out and opt.root, "usage: genids.lua --root <repo> --out <dir> --list <inputs.txt> [--strict]" )

local function norm( p ) return ( p:gsub( "\\", "/" ) ) end
local root = norm( opt.root ):gsub( "/$", "" ) .. "/"

local function readFile( p )
  local f = io.open( p, "rb" )
  if not f then return nil end
  local s = f:read( "*a" )
  f:close()
  return s
end

-- ---- loading Lua data files without the engine ----
local loadChunk
if setfenv then
  loadChunk = function( src, name, env )
    local f, err = loadstring( src, name )
    if f then setfenv( f, env ) end
    return f, err
  end
else
  loadChunk = function( src, name, env ) return load( src, name, "t", env ) end
end
local function stub()
  return setmetatable( {}, { __index = function() return stub() end, __call = function() return stub() end,
                             __concat = function() return "" end } )
end
local function loadData( path )
  local src = readFile( path )
  if not src then return nil end
  local env = setmetatable( { require = function() return stub() end }, { __index = function( _, k )
    local g = _G[k]
    if g ~= nil then return g end
    return stub()
  end } )
  local f, err = loadChunk( src, "@" .. path, env )
  if not f then return nil, err end
  local ok, v = pcall( f )
  if ok and type( v ) == "table" then return v end
  return nil, ok and "it doesn't return its table" or ( type( v ) == "string" and v or "it raised an error" )
end

-- ---- inputs ----
local files = {}
for line in ( readFile( opt.list ) or "" ):gmatch( "[^\r\n]+" ) do files[#files + 1] = norm( line ) end
table.sort( files )

-- kind -> set of names; kind order is the order of ids.h
local KINDS = { "anim", "scene", "spawn", "door", "song", "soundfont", "sound", "color", "coll", "edge", "genome",
                "model", "level" }
local DOC = {
  anim = "animation tags (Aseprite tags; DEFAULT for untagged frames)",
  scene = "scenes (genome/scene/<name>.lua)",
  spawn = "spawn points (scene.spawns / world3d.spawns keys)",
  door = "doors (<scene>><target>[:<spawn>])",
  song = "songs (bare names of genome/music/**.mid)",
  soundfont = "SoundFonts (bare names of genome/music/**.sf2)",
  sound = "sound effects (the keys of every genome's sfx.sounds; names are local to each genome)",
  color = "terminal colors (ColorPalette.h enum Color)",
  coll = "collision types (Genome.lua COLL; explicit values)",
  edge = "graph edge names (edges = { <name> = ... } tables)",
  genome = "genomes (genome/<name>.lua)",
  model = "genome models (bare names of genome/body/model/**.obj)",
  level = "3D levels (bare names of genome/level/**.lua, from tools/blender)",
}

-- Where each kind of bare-name reference lives (scripts/GenomeLayout.lua, the engine's single source of paths).
local Layout
do
  local here = ( debug and debug.getinfo and debug.getinfo( 1, "S" ).source or "" ):match( "^@(.*[/\\])" )
  for _, p in ipairs( { root .. "scripts/GenomeLayout.lua", here and ( here .. "../../scripts/GenomeLayout.lua" ) } ) do
    local src = p and readFile( p )
    if src then Layout = assert( loadChunk( src, "@" .. p, _G ) )() break end
  end
  assert( Layout, "genids: can't find scripts/GenomeLayout.lua" )
end
local REF_KINDS = { "soundfont", "song", "model", "level" }
local sets = {}
for _, k in ipairs( KINDS ) do sets[k] = {} end
local fixed = { coll = {}, color = {} }  -- kinds whose values come from a source, not sorting
local function add( kind, name )
  if type( name ) == "string" and name ~= "" then sets[kind][name] = true end
end

local errors, warnings = {}, {}
local function problem( msg ) errors[#errors + 1] = msg end

local games = {}  -- game -> { scenes = { name -> tbl }, music = { path -> true }, genomes = {}, sprites = {} }
local function game( g )
  if not games[g] then
    games[g] = { scenes = {}, music = {}, genomes = {}, bodies = {}, anims = {}, srcRects = {}, sources = {},
                 levels = {}, found = { soundfont = {}, song = {}, model = {}, level = {} }, genomeSrc = {}, unloadable = {} }
  end
  return games[g]
end

add( "anim", "DEFAULT" )
add( "spawn", "default" )

local function keys( t, kind )
  if type( t ) == "table" then
    for k in pairs( t ) do add( kind, k ) end
  end
end

for _, path in ipairs( files ) do
  local rel = path:sub( 1, #root ) == root and path:sub( #root + 1 ) or path
  local g, sub = rel:match( "^([^/]+)/genome/(.+)$" )
  local base = rel:match( "([^/]+)$" )
  if rel:match( "ColorPalette%.h$" ) then
    local src = readFile( path ) or ""
    local body = src:match( "enum%s+Color%s*{(.-)}" ) or ""
    local i = 0
    for name in body:gmatch( "([%a_][%w_]*)" ) do
      fixed.color[name] = i
      add( "color", name )
      i = i + 1
    end
  elseif rel:match( "^scripts/Genome%.lua$" ) then
    local src = readFile( path ) or ""
    local body = src:match( "local%s+COLL%s*=%s*{(.-)}" ) or ""
    for name, v in body:gmatch( "([%a_][%w_]*)%s*=%s*(%d+)" ) do
      fixed.coll[name] = tonumber( v )
      add( "coll", name )
    end
  end
  if g then
    local G = game( g )
    -- bare-name references: which file each name is (a name found twice in one game is ambiguous)
    for _, kind in ipairs( REF_KINDS ) do
      local n = Layout.nameOf( kind, sub )
      if n then
        local canonical = kind == "model" and ( sub:gsub( "%.%a+$", "" ) .. Layout.REFS.model.exts[1] ) or sub
        local prev = G.found[kind][n]
        if prev and prev ~= canonical then
          problem( g .. "/genome/" .. sub .. ": " .. kind .. " name '" .. n .. "' is ambiguous (also " .. g .. "/genome/" .. prev .. "); rename one" )
        end
        G.found[kind][n] = canonical
        add( kind, n )
      end
    end
    if sub:match( "^scene/[^/]+%.lua$" ) then
      local name = sub:match( "^scene/(.+)%.lua$" )
      add( "scene", name )
      local t = loadData( path )
      G.scenes[name] = t or {}
      G.sources[#G.sources + 1] = path
      if t then
        keys( t.spawns, "spawn" )
        if type( t.world3d ) == "table" then keys( t.world3d.spawns, "spawn" ) end
        if type( t.sfx ) == "table" then
          keys( t.sfx.sounds, "sound" )
          warnings[#warnings + 1] = path .. ": scene-level sfx is deprecated: declare each sound in its genome's sfx gene"
        end
        if type( t.world3d ) == "table" and type( t.world3d.entities ) == "table" then G.levels["@" .. name] = { entities = t.world3d.entities } end
      end
    elseif sub:match( "^level/.+%.lua$" ) then
      local t = loadData( path )
      local lname = Layout.nameOf( "level", sub )
      if t and lname then
        G.levels[lname] = t
        keys( t.spawns, "spawn" )
      end
    elseif sub:match( "^music/.+%.midi?$" ) or sub:match( "^music/.+%.sf2$" ) then
      G.music[sub] = true  -- old-style path references still resolve
    elseif sub:match( "_globalAnim%.lua$" ) then
      local t = loadData( path )
      if t then keys( t.strips, "anim" ); G.anims[base:gsub( "_globalAnim%.lua$", "" )] = t.strips or {} end
    elseif sub:match( "_anim_src_rects%.lua$" ) then
      local t = loadData( path )
      if t then keys( t.strips, "anim" ); G.srcRects[base:gsub( "%.lua$", "" )] = t.strips or {} end
    elseif sub:match( "^body/[^/]+%.lua$" ) then
      G.bodies[base:gsub( "%.lua$", "" )] = loadData( path ) or {}
    elseif sub:match( "^[^/]+%.lua$" ) then
      local name = sub:gsub( "%.lua$", "" )
      add( "genome", name )
      local gt, err = loadData( path )
      -- A genome that doesn't load (a syntax error) can't be checked: say so, not that its sounds etc. are missing.
      if not gt then
        problem( "genome doesn't load: " .. ( err or path ) )
        G.unloadable[name] = true
      end
      gt = gt or {}
      G.genomes[name] = gt
      G.genomeSrc[name] = path
      G.sources[#G.sources + 1] = path
      if type( gt.sfx ) == "table" then keys( gt.sfx.sounds, "sound" ) end
    end
    if sub:match( "%.lua$" ) then
      local src = readFile( path ) or ""
      for block in src:gmatch( "edges%s*=%s*(%b{})" ) do
        for name in block:gmatch( "([%a_][%w_]*)%s*=" ) do add( "edge", name ) end
      end
    end
  elseif rel:match( "^[^/]+/main%.lua$" ) then
    local G = game( rel:match( "^([^/]+)/" ) )
    G.sources[#G.sources + 1] = path
  end
end

-- The scene a level belongs to (world3d.level = "<level>"), for door names and messages.
local function levelOf( t )
  return type( t ) == "table" and type( t.world3d ) == "table" and type( t.world3d.level ) == "string" and
         ( Layout.isPath( t.world3d.level ) and Layout.nameOf( "level", t.world3d.level ) or t.world3d.level ) or nil
end

-- doors: 2D (fg entries with door = {...}), 3D levels (entities placed in Blender, the same entry shape) and
-- W.door{ to = ..., spawn = ... } in scene/main sources
for gname, G in pairs( games ) do
  for sname, t in pairs( G.scenes ) do
    local lists = {}
    for _, list in pairs( type( t.fg ) == "table" and t.fg or {} ) do lists[#lists + 1] = list end
    local lv = levelOf( t )
    if lv and G.levels[lv] and type( G.levels[lv].entities ) == "table" then lists[#lists + 1] = G.levels[lv].entities end
    if G.levels["@" .. sname] then lists[#lists + 1] = G.levels["@" .. sname].entities end
    for _, list in ipairs( lists ) do
      for _, e in ipairs( type( list ) == "table" and list or {} ) do
        if type( e ) == "table" then
          local proto = type( e.genome ) == "string" and G.genomes[e.genome] or {}
          if type( e.door ) == "table" or type( proto.door ) == "table" then
            local d = {}
            for k, v in pairs( type( proto.door ) == "table" and proto.door or {} ) do d[k] = v end
            for k, v in pairs( type( e.door ) == "table" and e.door or {} ) do d[k] = v end
            add( "door", d.name or ( sname .. ">" .. tostring( d.to ) .. ( d.spawn and ( ":" .. d.spawn ) or "" ) ) )
            G.doors = G.doors or {}
            G.doors[#G.doors + 1] = { from = sname, to = d.to, spawn = d.spawn }
          end
          if type( e.genome ) == "string" and not G.genomes[e.genome] then
            problem( gname .. "/genome/scene/" .. sname .. ".lua: no genome '" .. e.genome .. "'" )
          end
        end
      end
    end
  end
  for _, path in ipairs( G.sources ) do
    local src = readFile( path ) or ""
    local from = path:match( "/scene/([^/]+)%.lua$" ) or "?"
    for block in src:gmatch( "W%.door%s*(%b{})" ) do
      local to = block:match( "to%s*=%s*\"([^\"]+)\"" )
      local spawn = block:match( "spawn%s*=%s*\"([^\"]+)\"" )
      add( "door", from .. ">" .. tostring( to ) .. ( spawn and ( ":" .. spawn ) or "" ) )
      G.doors = G.doors or {}
      G.doors[#G.doors + 1] = { from = from, to = to, spawn = spawn, file = path }
    end
    for _, pat in ipairs( { "Scene%.go%s*%(%s*\"([^\"]+)\"", "Scene%.goTo%s*%(%s*\"([^\"]+)\"",
                            "Scene%[%s*[\"']goto[\"']%s*%]%s*%(%s*\"([^\"]+)\"", "Scene%.start%s*%(%s*\"([^\"]+)\"" } ) do
      for name in src:gmatch( pat ) do
        if not G.scenes[name] then problem( path .. ": scene '" .. name .. "' doesn't exist" ) end
      end
    end
    for block in src:gmatch( "sounds%s*=%s*(%b{})" ) do  -- sfx tables outside scene files (main.lua)
      for name in block:gmatch( "([%a_][%w_]*)%s*=" ) do add( "sound", name ) end
    end
  end
  for _, path in ipairs( G.sources ) do
    local src = readFile( path ) or ""
    for name in src:gmatch( "Sound%.play%s*%(%s*\"([^\"]+)\"" ) do
      if not sets.sound[name] then problem( path .. ": Sound.play: no sound '" .. name .. "' in any genome's sfx.sounds" ) end
    end
    for name in src:gmatch( "Sound%.key%s*%(%s*\"([^\"]+)\"" ) do
      if not sets.sound[name] then problem( path .. ": Sound.key: no sound '" .. name .. "' in any genome's sfx.sounds" ) end
    end
  end
end

-- ---- checks ----
local function spawnsOf( t, G )
  local s = {}
  if type( t ) ~= "table" then return s end
  local lv = levelOf( t )
  if lv and G and G.levels[lv] and type( G.levels[lv].spawns ) == "table" then
    for k in pairs( G.levels[lv].spawns ) do s[k] = true end
  end
  for k in pairs( type( t.spawns ) == "table" and t.spawns or {} ) do s[k] = true end
  if type( t.world3d ) == "table" and type( t.world3d.spawns ) == "table" then
    for k in pairs( t.world3d.spawns ) do s[k] = true end
  end
  return s
end
-- A bare-name reference (or an old genome-relative path) that has to be a file of the game.
local function checkRef( G, gname, where, kind, ref )
  if type( ref ) ~= "string" then return end
  if Layout.isPath( ref ) then
    if not ( G.music[ref] or ref:match( "^sf2/" ) or G.found[kind][Layout.nameOf( kind, ref ) or ""] ) then
      problem( where .. ": " .. kind .. " '" .. ref .. "' isn't a file of " .. gname .. "/genome/" )
    end
    return
  end
  if not G.found[kind][ref] then
    local list = {}
    for n in pairs( G.found[kind] ) do list[#list + 1] = n end
    table.sort( list )
    local r = Layout.REFS[kind]
    problem( where .. ": no " .. r.what .. " '" .. ref .. "' (" .. gname .. "/genome/" .. r.dir .. ref .. ( kind == "model" and ".obj" or r.exts[1] ) ..
             "); " .. gname .. " has: " .. ( #list > 0 and table.concat( list, ", " ) or "none" ) )
  end
end

for gname, G in pairs( games ) do
  -- genomes: their own sounds, models and SoundFonts
  for gn, t in pairs( G.genomes ) do
    local where = gname .. "/genome/" .. gn .. ".lua"
    local own = {}
    if type( t.sfx ) == "table" then
      checkRef( G, gname, where, "soundfont", t.sfx.soundfont )
      for k, v in pairs( type( t.sfx.sounds ) == "table" and t.sfx.sounds or {} ) do
        own[k] = true
        if type( v ) == "table" then checkRef( G, gname, where, "soundfont", v.soundfont ) end
      end
    end
    if type( t.model ) == "string" then checkRef( G, gname, where, "model", t.model ) end
    local src = not G.unloadable[gn] and readFile( G.genomeSrc[gn] ) or ""
    for name in src:gmatch( "self:sound%s*%(%s*\"([^\"]+)\"" ) do
      if not own[name] then problem( where .. ": self:sound: no sound '" .. name .. "' in this genome's sfx.sounds" ) end
    end
  end
  -- levels: the genomes placed in them
  for lname, lt in pairs( G.levels ) do
    for _, e in ipairs( type( lt.entities ) == "table" and lt.entities or {} ) do
      if type( e ) == "table" and type( e.genome ) == "string" and not G.genomes[e.genome] then
        problem( gname .. "/genome/level/" .. lname .. ".lua: no genome '" .. e.genome .. "' (placed as " .. tostring( e.key ) .. ")" )
      end
    end
  end
  for _, d in ipairs( G.doors or {} ) do
    local where = d.file or ( gname .. "/genome/scene/" .. d.from .. ".lua" )
    if not G.scenes[d.to] then
      problem( where .. ": door to scene '" .. tostring( d.to ) .. "', which doesn't exist" )
    elseif d.spawn and d.spawn ~= "default" and not spawnsOf( G.scenes[d.to], G )[d.spawn] then
      problem( where .. ": door to " .. d.to .. " uses spawn '" .. d.spawn .. "', which " .. d.to .. " doesn't have" )
    end
  end
  for sname, t in pairs( G.scenes ) do
    local where = gname .. "/genome/scene/" .. sname .. ".lua"
    for _, m in ipairs( { t.music, t.sfx } ) do
      if type( m ) == "table" then
        for _, f in ipairs( { "song", "soundfont" } ) do checkRef( G, gname, where, f, m[f] ) end
      end
    end
    if type( t.world3d ) == "table" and t.world3d.level then checkRef( G, gname, where, "level", t.world3d.level ) end
    if type( t.music ) == "table" and t.music.transition and not ( { crossfade = 1, cut = 1, keep = 1 } )[t.music.transition] then
      problem( where .. ": music.transition must be crossfade, cut or keep" )
    end
  end
  for gn, t in pairs( G.genomes ) do
    local where = gname .. "/genome/" .. gn .. ".lua"
    if type( t.coll ) == "table" and type( t.coll.type ) == "string" and not fixed.coll[t.coll.type] then
      problem( where .. ": unknown collision type '" .. t.coll.type .. "'" )
    end
    if type( t.initAnim ) == "string" and type( t.sprite ) == "string" then
      local body = G.bodies[t.sprite]
      local tags = {}
      if body then
        if body.anim and G.anims[body.anim] then for k in pairs( G.anims[body.anim] ) do tags[k] = true end end
        for _, l in ipairs( type( body.layers ) == "table" and body.layers or {} ) do
          if type( l ) == "table" and l.animSrcRects and G.srcRects[l.animSrcRects] then
            for k in pairs( G.srcRects[l.animSrcRects] ) do tags[k] = true end
          end
        end
        if next( tags ) and not tags[t.initAnim] then
          problem( where .. ": initAnim '" .. t.initAnim .. "' isn't a tag of sprite " .. t.sprite )
        end
      end
    end
  end
end
-- collision palettes name collision types
for _, path in ipairs( files ) do
  if path:match( "/body/palette/collision/[^/]+%.lua$" ) then
    local t = loadData( path )
    if t and type( t.types ) == "table" then
      for _, v in pairs( t.types ) do
        if type( v ) == "string" and not fixed.coll[v] then problem( path .. ": unknown collision type '" .. v .. "'" ) end
      end
    end
  end
end

-- ---- ids ----
local ids, names = {}, {}
for _, kind in ipairs( KINDS ) do
  ids[kind], names[kind] = {}, {}
  if fixed[kind] and next( fixed[kind] ) then
    for name, v in pairs( fixed[kind] ) do ids[kind][name] = v; names[kind][v] = name end
  else
    local list = {}
    for name in pairs( sets[kind] ) do list[#list + 1] = name end
    table.sort( list, function( a, b )
      if a == "DEFAULT" or a == "default" then return b ~= a end  -- the default first: id 0
      if b == "DEFAULT" or b == "default" then return false end
      return a < b
    end )
    if #list > 255 then problem( kind .. ": " .. #list .. " names; ids are uint8_t (at most 255)" ) end
    for i, name in ipairs( list ) do ids[kind][name] = i - 1; names[kind][i - 1] = name end
  end
end

for _, w in ipairs( warnings ) do io.stderr:write( "genids: warning: " .. w .. "\n" ) end
if #errors > 0 then
  table.sort( errors )
  for _, e in ipairs( errors ) do io.stderr:write( "genids: " .. ( opt.strict and "error: " or "warning: " ) .. e .. "\n" ) end
  if opt.strict then os.exit( 1 ) end
end

-- ---- output ----
local CXX_KEYWORDS = {}
for w in ( "alignas alignof and and_eq asm auto bitand bitor bool break case catch char char8_t char16_t char32_t " ..
           "class compl concept const consteval constexpr constinit const_cast continue co_await co_return co_yield " ..
           "decltype default delete do double dynamic_cast else enum explicit export extern false float for friend " ..
           "goto if inline int long mutable namespace new noexcept not not_eq nullptr operator or or_eq private " ..
           "protected public register reinterpret_cast requires return short signed sizeof static static_assert " ..
           "static_cast struct switch template this thread_local throw true try typedef typeid typename union " ..
           "unsigned using virtual void volatile wchar_t while xor xor_eq NULL" ):gmatch( "%S+" ) do
  CXX_KEYWORDS[w] = true
end
local function cname( s )
  local c = s:gsub( "[^%w_]", "_" )
  if c:match( "^%d" ) then c = "_" .. c end
  if c == "COUNT" or CXX_KEYWORDS[c] then c = c .. "_" end
  return c
end
local function maxId( kind )
  local m = -1
  for _, v in pairs( ids[kind] ) do if v > m then m = v end end
  return m
end
local function q( s ) return string.format( "%q", s ) end

local h = { "// Generated by tools/ids/genids.lua from every game's genome tree: don't edit (README, \"IDs\").",
            "#pragma once", "#include <cstdint>", "", "namespace ids {", "" }
for _, kind in ipairs( KINDS ) do
  local T = kind:sub( 1, 1 ):upper() .. kind:sub( 2 )
  local used, order = {}, {}
  for id = 0, maxId( kind ) do
    if names[kind][id] then order[#order + 1] = id end
  end
  h[#h + 1] = "// " .. DOC[kind]
  h[#h + 1] = "enum class " .. T .. " : uint8_t {"
  for _, id in ipairs( order ) do
    local c = cname( names[kind][id] )
    while used[c] do c = c .. "_" end
    used[c] = true
    h[#h + 1] = "  " .. c .. " = " .. id .. ","
  end
  h[#h + 1] = "  COUNT = " .. ( maxId( kind ) + 1 )
  h[#h + 1] = "};"
  local tbl = {}
  for id = 0, maxId( kind ) do tbl[#tbl + 1] = names[kind][id] and q( names[kind][id] ) or "nullptr" end
  h[#h + 1] = "inline constexpr const char* " .. kind .. "Names[] = { " .. ( #tbl > 0 and table.concat( tbl, ", " ) or "nullptr" ) .. " };"
  h[#h + 1] = "inline constexpr unsigned " .. kind .. "Count = " .. ( maxId( kind ) + 1 ) .. ";"
  h[#h + 1] = ""
end
h[#h + 1] = "}  // namespace ids"
h[#h + 1] = ""

local l = { "-- Generated by tools/ids/genids.lua from every game's genome tree: don't edit (README, \"IDs\").",
            "-- ids.<kind>[name] = id (0-based); ids.names.<kind>[id] = name.", "local ids = { names = {} }" }
for _, kind in ipairs( KINDS ) do
  local list = {}
  for id = 0, maxId( kind ) do
    if names[kind][id] then list[#list + 1] = id end
  end
  local a, b = {}, {}
  for _, id in ipairs( list ) do
    a[#a + 1] = "[" .. q( names[kind][id] ) .. "]=" .. id
    b[#b + 1] = "[" .. id .. "]=" .. q( names[kind][id] )
  end
  l[#l + 1] = "ids." .. kind .. " = { " .. table.concat( a, ", " ) .. " }"
  l[#l + 1] = "ids.names." .. kind .. " = { " .. table.concat( b, ", " ) .. " }"
end
-- Which file each bare name is, per game (scripts/GenomeLayout.lua's Layout.path uses it).
l[#l + 1] = "ids.paths = {"
do
  local gnames = {}
  for gname in pairs( games ) do gnames[#gnames + 1] = gname end
  table.sort( gnames )
  for _, gname in ipairs( gnames ) do
    local parts = {}
    for _, kind in ipairs( REF_KINDS ) do
      local ns = {}
      for n in pairs( games[gname].found[kind] ) do ns[#ns + 1] = n end
      table.sort( ns )
      local items = {}
      for _, n in ipairs( ns ) do items[#items + 1] = "[" .. q( n ) .. "]=" .. q( games[gname].found[kind][n] ) end
      parts[#parts + 1] = kind .. " = { " .. table.concat( items, ", " ) .. " }"
    end
    l[#l + 1] = "  [" .. q( gname ) .. "] = { " .. table.concat( parts, ", " ) .. " },"
  end
end
l[#l + 1] = "}"
l[#l + 1] = "return ids"
l[#l + 1] = ""

local function writeIfChanged( p, s )
  if readFile( p ) == s then return end
  local f = assert( io.open( p, "wb" ), "genids: can't write " .. p )
  f:write( s )
  f:close()
  print( "genids: wrote " .. p )
end
local out = norm( opt.out ):gsub( "/$", "" ) .. "/"
writeIfChanged( out .. "ids.h", table.concat( h, "\n" ) )
writeIfChanged( out .. "ids.lua", table.concat( l, "\n" ) )
if opt.stamp then
  local f = io.open( opt.stamp, "wb" )
  if f then f:write( "ok\n" ); f:close() end
end
