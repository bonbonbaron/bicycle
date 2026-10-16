require("Window")
require("io")
require("table")

-- Layer etc. aren't going to be needed anywhere else, so I'll put them all in here.

local Scene = {}
Scene.__index = Scene
Scene.__newindex = function() error( "You can't create new attributes for Scene." ) end

-- Module state (declared up front, never set to nil: Scene inherits Window, which forbids new keys).
Scene.current = false       -- the scene instance on screen
Scene.file = false          -- its file name under genome/scene/ (what Scene.go takes)
Scene.state = { removed = {} }  -- its saved state (see Scene.stateOf); scripts may keep anything in it
Scene.persistent = {}       -- genome name -> instance of every persist = true entity alive (hero, fairy, ...)
Scene.live = {}             -- this scene's own (non-persistent) instances
Scene.carriedLayer = {}     -- persistent genome name -> its layer in this scene
Scene.playerName = false    -- the genome the camera and input focus on (the fg entry with focus = true)
Scene.frameNo = 0           -- frames since start (counted at the end of each frame)
Scene.enteredFrame = 0      -- frame the current scene was entered
Scene.musicMode = "crossfade"  -- how the scene being entered treats music (Scene.go's opts.music)

function Scene:getFileIter(path)
  -- Open a pipe to read ("r") from the command.
  local isWindows = package.config:sub(1, 1) == "\\"
  local handle = io.popen(isWindows and ('dir /b "'..path:gsub("/", "\\")..'"') or ("ls "..path))
  if not handle then
    error("failed to execute ls for "..self.name)
  end
  return handle
end

function dbgp( tbl )
  for key, val in pairs(tbl) do
    print( tostring(key)..": "..tostring(val) )  -- tall order...
  end
  io.read()
end

-- BG will be asterisked. You have to specify which layer you want your entities on.
function Scene:getBg( path )
  local bg = {}
  checkType(path ,"string")
  local packed = Asset.list( ( path:gsub( "/%*%.txt$", "" ) ) )
  if packed then
    for _, fp in ipairs( packed ) do
      if fp:match( "%.txt$" ) then table.insert( bg, Asset.read( fp ) ) end
    end
    if #bg == 0 then error("Found no files in "..path) end
    return bg
  end
  handle = self:getFileIter( path )
  -- Process each file found above with callback argument.
  numFiles = 0
  for fp in handle:lines() do
    local file = io.open( fp, "r" )
    local contents = file:read("*a")
    table.insert( bg, contents )
    file:close()
    numFiles = numFiles + 1
  end
  handle:close()
  if numFiles == 0 then error("Found no files in "..path) end
  return bg
end

-- \ TODO revive BG
-- TODO impl FG

-- Spawns a background layer's entities. fg entries: { genome = "coin", pos = { x, y }, focus = true,
-- persist = true, key = "<id for saved state>", <any genome field to override, e.g. door = {...}> }.
-- Persistent genomes already alive (carried from the last scene) aren't spawned again: their entry only says which
-- layer they belong to here. Entries a script removed for good (Scene.removeForever) stay gone.
function Scene:addFg( Genome, fg, layerIdx )
  checkType( fg, "table" )
  local spawned = {}
  for _, instance in ipairs( fg ) do
    local key = Scene.keyOf( instance )
    local proto = Genome.get( instance.genome )
    local persist = instance.persist or proto.persist
    if Scene.persistent[instance.genome] then
      Scene.carriedLayer[instance.genome] = Scene.carriedLayer[instance.genome] or layerIdx
      if instance.focus then Scene.playerName = instance.genome end
    elseif not Scene.state.removed[key] then
      -- Scene-owned entities load into the SCENE pool; persistent ones into the long-lived pools.
      local genome = Scene.withScope( not persist, function()
        local overrides = {}
        for k, v in pairs( instance ) do
          if k ~= "genome" and k ~= "pos" and k ~= "focus" and k ~= "key" then overrides[k] = v end
        end
        return Genome.new( instance.genome, overrides )
      end )
      genome.key, genome.name, genome.layer = key, instance.genome, layerIdx
      table.insert( spawned, genome )
      Genome.byName[instance.genome] = Genome.byName[instance.genome] or genome.id
      if not genome.tbl.door then ffi.C.addFgEntity( genome.id ) end  -- doors are invisible: not drawn at all
      if instance.pos then
        ffi.C.setPos( genome.id, ffi.new( "Position", { x = instance.pos[1], y = instance.pos[2], z = instance.pos[3] or 0 } ) )
        local boxes = ffi.C.World_getBoxes()
        genome.tbl.pos = boxes.arr[genome.id].pos
        genome.tbl.size = boxes.arr[genome.id].size
      end
      genome.tbl.id = genome.id
      if persist then
        Scene.persistent[instance.genome] = genome
        Scene.carriedLayer[instance.genome] = layerIdx
      else
        table.insert( Scene.live, genome )
      end
      if instance.focus then
        Scene.playerName = instance.genome
        focusOn( genome.id )
        ffi.C.focusCamOn( genome.id )
      end
      register( genome.id, genome.tbl )
      Scene.withScope( not persist, function()
        if genome.tbl.onSpawn then genome.tbl:onSpawn() end
      end )
    end
  end
  -- Motions last: a fairy may follow the hero spawned after it.
  for _, genome in ipairs( spawned ) do Genome.startMotion( genome ) end
end

-- bg is either an Aseprite background sprite exported by scripts/aseprite/sprite.lua (one tilemap layer per
-- background layer, each with its mode) or a directory of .txt layers under genome/scene/bg/.
-- fg[i] holds the entities that live in background layer i.
function Scene:readLayers(Genome)
  if not self.bg then return end  -- e.g. a true-3D scene: no background layers
  local Sprite = Genome.Sprite
  if Sprite.exists( self.bg ) then
    local spr = Sprite.load( self.bg )
    if spr.kind ~= Sprite.TILEMAP then error( "Scene background '"..self.bg.."' isn't a tilemap sprite." ) end
    for bgIdx, tm in ipairs( spr.layers ) do
      Sprite.addBgLayer( tm )
      if self.fg[bgIdx] then
        self:addFg( Genome, self.fg[bgIdx], bgIdx )
      end
    end
    return
  end
  -- TODO later don't require backgrounds to allow for entities without 
  local bg = self:getBg( "./"..gameName.."/genome/scene/bg/"..self.bg.."/*.txt"  )
  local bgIdx = 1
  for _, layer in pairs(bg) do
    ffi.C.addBgLayer( layer )
    if self.fg[bgIdx] then
      self:addFg( Genome, self.fg[bgIdx], bgIdx )  -- bg in the layer level specifies which bg these entities belong in
    end
    bgIdx = bgIdx + 1
  end
end

function Scene.new( tbl )
  checkType( tbl, "table" )
  local id = ffi.C.pushScene()  -- bicycle::pushScene() returns an entity ID of that scene
  local instance = { name=tbl.name, id=id, bg=tbl.bg, fg=tbl.fg, file=tbl.file }
  local Genome = require("Genome")
  setmetatable( instance, Scene )  -- inherit Scene
  setmetatable( Scene, Window )    -- have Scene in turn inherit Window (its supertype)
  Window.register(id, instance)
  Scene.current = instance
  Scene.file = tbl.file or Scene.file
  Scene.state = Scene.stateOf( Scene.file or tbl.name or "?" )
  Scene.withScope( true, function()
    require("Sound").load( tbl.music, tbl.sfx, Scene.musicMode == "keep" )
    instance:readLayers(Genome)
    if tbl.world3d then instance:loadWorld3d( tbl ) else instance:loadScenery3d(Genome, tbl) end
  end )
  Scene.enteredFrame = Scene.frameNo
  return instance
end

-- True-3D scenes (docs/true-3d.md): tbl.world3d = { level = "level/scene3d.lua" (collision + drawing),
--   sky = { r, g, b }, fog = { r, g, b, near, far }, light = { dir, color, ambient, shadows },
--   spawns = { <name> = { x, y, z, yaw } }, build = function( scene ) ... end  (makes the scene's own bodies,
--   doors (W.door) and anything else) }. Persistent bodies (W.Body.new{ persist = true }) are carried over.
Scene.world3d = false  -- the current scene's world3d table (false for 2D / 2.5D scenes)
function Scene:loadWorld3d( tbl )
  local W = require( "World3D" )
  local w = tbl.world3d
  if w.level then W.level( "./"..gameName.."/genome/"..w.level, w.cell ) end
  if w.sky then W.View.sky( w.sky[1], w.sky[2], w.sky[3] ) end
  if w.fog then W.View.fog( w.fog[1], w.fog[2], w.fog[3], w.fog[4], w.fog[5] ) end
  if w.light then W.View.light( w.light[1], w.light[2], w.light[3], w.light[4] ) end
  Scene.world3d = w
  if w.build then w.build( self ) end
end

-- Starts the game in genome/scene/<name>.lua (instead of Scene.new on a table you required yourself).
function Scene.start( name )
  local tbl = require( "./"..gameName.."/genome/scene/"..name )
  tbl.file = name
  return Scene.new( tbl )
end

-- 3D mode: static scenery from scripts/blender/export.py's scene3d.lua (ground, props), or the file the scene names
-- with scenery3d = "<name>" (level/<name>.lua; false for none). Characters in it are placed by their genomes'
-- model genes instead.
function Scene:loadScenery3d(Genome, tbl)
  if Genome.Sprite.renderMode() ~= "3d" then return end
  if tbl and tbl.scenery3d == false then return end
  local dir = "./"..gameName.."/genome/"
  local file = dir.."level/"..( tbl and tbl.scenery3d or "scene3d" )..".lua"
  if not Asset.exists( file ) then return end
  local scenery = Asset.dofile( file )
  for _, obj in ipairs( scenery.objects ) do
    if obj.role ~= "character" and not ffi.C.addStaticModel( dir..obj.mesh..".bmesh" ) then
      error( "Couldn't load 3D scenery '"..obj.mesh.."' (build the 'assets' target)." )
    end
  end
end

-- TODO function Scene::remove(), good for item menus in RPGs

function Scene:onInput( input )
  if input == ffi.C.Q then
    self.pop(self.id)
  end
end

--------------------
---- SCENE CHANGES
--------------------
-- Scene.go( name, opts ) leaves this scene for genome/scene/<name>.lua at the end of the frame:
--   opts = { spawn = "<spawn point>", transition = "fade" | "wipe" | "dissolve" | "none", duration = 0.6 (s),
--            music = "crossfade" | "cut" | "keep", replace = false }
-- (Scene.goTo and Scene["goto"] are the same function; LuaJIT reserves the word goto.)
-- Lifecycle, always in this order, between frames:
--   1. the transition covers the screen (input is ignored until it's uncovered; music fades out)
--   2. dialogues/menus over the scene close; the scene's own entities get onDespawn and are despawned
--   3. the old scene's window (layers, camera, HUD) goes; the renderer drops what only it used; the SCENE memory
--      pool is reset (C++ checks that nothing in it is still alive: "leaked" in BICYCLE_SCENE_STATS)
--   4. the new scene loads (music, layers, entities, 3D scenery) into the SCENE pool
--   5. persistent entities join it: the player at the spawn point, its companions at the same offset from it, the
--      camera tracking it as before, the HUD rebuilt; each gets onSceneEnter( sceneName, spawnName )
--   6. the transition uncovers the screen; music fades in
-- Spawn points: scene.spawns = { default = { x, y }, entry = { x, y }, ... } (cells, the player's top-left). Without
-- one, the player's own fg entry (if the scene lists it) or the default is used. A missing spawn name is an error
-- in dev builds and falls back to the default (with a warning) in release builds.
-- Saved state: Scene.state is a table per scene file kept for the whole run (so a scene remembers things when
-- you come back). Scene.removeForever( self ) despawns an entity and keeps it out of that scene from then on.
local S = { pending = false, trans = false, nearDoor = false, removals = {}, cycle = false }
local SceneState = {}  -- scene file -> its state
local TRANSITIONS = { none = 0, fade = 1, wipe = 2, dissolve = 3 }
local FPS = 30
local DOOR_GRACE_FRAMES = 15  -- a door touched this soon after arriving waits for the player to step off it first

local scoped = false
function memSceneScoped() return scoped end
-- Runs fn with the SCENE-pool scope on (images, meshes and songs it loads belong to the scene) or off.
function Scene.withScope( on, fn )
  local prev = scoped
  if on ~= prev then ffi.C.sceneScope( on ) scoped = on end
  local ok, r = pcall( fn )
  if on ~= prev then ffi.C.sceneScope( prev ) scoped = prev end
  if not ok then error( r, 0 ) end
  return r
end

function Scene.keyOf( instance )
  if instance.key then return instance.key end
  local p = instance.pos or {}
  return instance.genome.."@"..tostring( p[1] )..","..tostring( p[2] )
end

function Scene.stateOf( file )
  SceneState[file] = SceneState[file] or { removed = {} }
  return SceneState[file]
end

local function sceneTable( name )
  local ok, tbl = pcall( require, "./"..gameName.."/genome/scene/"..name )
  if not ok or type( tbl ) ~= "table" then error( "Scene.go: no scene '"..tostring( name ).."' (genome/scene/"..tostring( name )..".lua): "..tostring( tbl ), 0 ) end
  tbl.file = name
  return tbl
end

function Scene.go( name, opts )
  checkType( name, "string" )
  opts = opts or {}
  if S.trans then  -- already changing scenes: only replace = true may still redirect it (before the swap)
    if opts.replace and S.trans.phase == "out" then S.trans.name, S.trans.opts = name, opts return true end
    return false
  end
  if S.pending and not opts.replace then return false end
  sceneTable( name )  -- fail now, in the caller, if it doesn't exist
  S.pending = { name = name, opts = opts }
  return true
end
Scene.goTo = Scene.go
Scene["goto"] = Scene.go

-- Reads the next scene's Lua tables ahead (the scene file and its genomes), so the change itself only decodes art.
function Scene.preload( name )
  local tbl = sceneTable( name )
  for _, layer in pairs( tbl.fg or {} ) do
    for _, instance in ipairs( layer ) do require( "Genome" ).get( instance.genome ) end
  end
  return true
end

function Scene.busy() return S.trans ~= false or S.pending ~= false end

-- Despawns now (callers must be between frames: Scene's own frame hook).
local function despawn( genome, gone )
  if genome.tbl.onDespawn then genome.tbl:onDespawn() end
  ffi.C.despawnEntity( genome.id )
  unregister( genome.id )
  if gone then gone[genome.id] = true end
  genome.tbl.despawned = true
  if S.nearDoor == genome.tbl then S.nearDoor = false end
  local speakers = Dialogue and Dialogue.speakers or {}
  for i = #speakers, 1, -1 do
    if speakers[i] == genome.tbl then table.remove( speakers, i ) end
  end
end

-- Takes an entity out of the scene for good (e.g. a picked-up key): despawned at the end of the frame, and not
-- spawned again when the player comes back.
function Scene.removeForever( tbl )
  for _, g in ipairs( Scene.live ) do
    if g.tbl == tbl then
      Scene.state.removed[g.key] = true
      table.insert( S.removals, g )
      return true
    end
  end
  return false
end

local function flushRemovals()
  if #S.removals == 0 then return end
  local gone = {}
  for _, g in ipairs( S.removals ) do
    for i, l in ipairs( Scene.live ) do
      if l == g then table.remove( Scene.live, i ) break end
    end
    despawn( g, gone )
  end
  S.removals = {}
  dropUncollisionsFor( function( id ) return gone[id] end )
end

local function boxOf( id ) return ffi.C.World_getBoxes().arr[id] end

local function spawnPoint( tbl, spawnName )
  local spawns = tbl.spawns or {}
  if spawnName and spawns[spawnName] then return spawns[spawnName] end
  if spawnName then
    local msg = "Scene.go: scene '"..tbl.file.."' has no spawn point '"..spawnName.."'"
    if not ffi.C.assetPacked() then error( msg, 0 ) end
    io.stderr:write( "bicycle: "..msg.."; using its default\n" )
  end
  if spawns.default then return spawns.default end
  for _, layer in pairs( tbl.fg or {} ) do
    for _, instance in ipairs( layer ) do
      if instance.genome == Scene.playerName and instance.pos then return instance.pos end
    end
  end
end

local function firstFgLayer( tbl )
  local k
  for idx in pairs( tbl.fg or {} ) do if not k or idx < k then k = idx end end
  return k or 1
end

-- Steps 2-5 of the lifecycle (see above).
local function swap( name, opts )
  local tbl = sceneTable( name )
  local oldName = Scene.file or "?"
  -- Where the persistent entities stand relative to the player, to keep it in the new scene.
  local player = Scene.playerName and Scene.persistent[Scene.playerName]
  local rel = {}
  if player then
    local pp = boxOf( player.id ).pos
    for pname, g in pairs( Scene.persistent ) do
      local p = boxOf( g.id ).pos
      rel[pname] = { ( p.x - pp.x ) / 256, ( p.y - pp.y ) / 256 }
    end
  end
  -- 2. windows over the scene, then the scene's own entities
  while not ffi.C.topIsScene() and ffi.C.topWindowId() ~= 0 do
    local wid = ffi.C.topWindowId()
    ffi.C.popWindow()
    unregister( wid )
  end
  local gone = {}
  for _, g in ipairs( Scene.live ) do despawn( g, gone ) end
  Scene.live, S.removals, S.nearDoor = {}, {}, false
  -- true 3D: the scene's bodies, then its level collision (persistent bodies stay)
  local W = package.loaded["World3D"]
  local rel3d = {}
  if W and type( W ) == "table" then
    if W.player then
      local px, py, pz = W.player:pos()
      for id, b in pairs( W.persistent ) do
        local x, y, z = b:pos()
        rel3d[id] = { x - px, y - py, z - pz }
      end
      local c = ffi.C.xfGet( ffi.C.cameraEntity() )  -- the camera keeps its place relative to the player
      rel3d.camera = { c.x - px, c.y - py, c.z - pz }
    end
    for _, b in ipairs( W.live ) do despawn( { id = b.id, tbl = b }, gone ) end
    W.live = {}
    ffi.C.levelClear()
  end
  Scene.world3d = false
  -- 3. the scene window, then its memory
  local old = Scene.current
  if old then
    ffi.C.popWindow()
    unregister( old.id )
  end
  local leaked = ffi.C.sceneUnloaded()
  ffi.C.sceneStats( "unload", oldName, leaked )
  if leaked ~= 0 then io.stderr:write( "bicycle: scene '"..oldName.."' left "..tostring( leaked ).." bytes alive in the SCENE pool\n" ) end
  dropUncollisionsFor( function( id ) return gone[id] end )
  -- 4. the new scene (persistent entities listed in it aren't spawned again)
  local Genome = require( "Genome" )
  Genome.byName = {}
  for pname, g in pairs( Scene.persistent ) do Genome.byName[pname] = g.id end
  Scene.carriedLayer = {}
  -- How the music changes: Scene.go's opts.music, else the entered scene's music.transition, else crossfade.
  Scene.musicMode = opts.music or ( tbl.music and tbl.music.transition ) or "crossfade"
  Scene.new( tbl )
  -- 5. persistent entities arrive
  local defaultLayer = firstFgLayer( tbl )
  for pname, g in pairs( Scene.persistent ) do
    ffi.C.attachEntity( g.id, ( Scene.carriedLayer[pname] or defaultLayer ) - 1 )
  end
  player = Scene.playerName and Scene.persistent[Scene.playerName]
  if player then
    local sp = spawnPoint( tbl, opts.spawn )
    if sp then
      for pname, g in pairs( Scene.persistent ) do
        local r = rel[pname] or { 0, 0 }
        ffi.C.setPos( g.id, ffi.new( "Position", { x = sp[1] + r[1], y = sp[2] + r[2], z = 0 } ) )
        ffi.C.motionSnap( g.id )  -- followers jump to their goal instead of flying across the new map
      end
    end
    focusOn( player.id )
    ffi.C.focusCamOn( player.id )
    if Camera.target == player.id and Camera.opts then Camera.track( player.id, Camera.opts ) end
  end
  -- true 3D: the player body on the spawn point, companions at their offsets, the camera rig again
  if W and type( W ) == "table" and W.player and tbl.world3d then
    local spawns = tbl.world3d.spawns or {}
    local sp = ( opts.spawn and spawns[opts.spawn] ) or spawns.default or select( 2, next( spawns ) )
    if opts.spawn and not spawns[opts.spawn] then
      local msg = "Scene.go: scene '"..name.."' has no spawn point '"..opts.spawn.."'"
      if not ffi.C.assetPacked() then error( msg, 0 ) end
      io.stderr:write( "bicycle: "..msg.."\n" )
    end
    if sp then
      for id, b in pairs( W.persistent ) do
        local r = rel3d[id] or { 0, 0, 0 }
        b:setPos( sp[1] + r[1], sp[2] + r[2], sp[3] + r[3] )
        b:setVel( 0, 0, 0 )
      end
      if sp[4] then W.player:setYaw( sp[4] ) end
    end
    focusOn( W.player.id )
    local px, py, pz = W.player:pos()
    local rc = rel3d.camera or { 0, 2, 6 }
    ffi.C.xfSet( ffi.C.cameraEntity(), px + rc[1], py + rc[2], pz + rc[3] )
    W.Camera.reapply()
  end
  if W and type( W ) == "table" and not tbl.world3d and W.Camera.last then W.Camera.off() end  -- back to 2.5D
  require( "Hud" ).rebuild()
  for _, b in pairs( W and type( W ) == "table" and W.persistent or {} ) do
    if b.onSceneEnter then b:onSceneEnter( name, opts.spawn ) end
  end
  for _, g in pairs( Scene.persistent ) do
    if g.tbl.onSceneEnter then g.tbl:onSceneEnter( name, opts.spawn ) end
  end
  ffi.C.sceneStats( "enter", name, 0 )
end

-- Doors (the door gene in Genome.lua): the zone's collision callbacks.
function Scene.armDoor( tbl )
  local HERO = 1  -- Genome.COLL.hero
  tbl.onCollision = function( self, other, collType )
    if collType ~= HERO or self.despawned then return end
    if Scene.frameNo - Scene.enteredFrame < DOOR_GRACE_FRAMES then self.waiting = true return end
    if self.waiting then return end
    if ( self.door.on or "touch" ) == "press" then S.nearDoor = self return end
    Scene.useDoor( self )
  end
  tbl.onUncollision = function( self, other, collType )
    if collType ~= HERO then return end
    self.waiting = false
    if S.nearDoor == self then S.nearDoor = false end
  end
end

-- Goes through door (a door genome's table), or the "press" door the player stands in; false if none.
function Scene.useDoor( door )
  door = door or S.nearDoor
  if not door or door.despawned then return false end
  local d = door.door
  return Scene.go( d.to, { spawn = d.spawn, transition = d.transition, duration = d.duration, music = d.music } )
end

local function startTransition( p )
  local opts = p.opts
  local kind = opts.transition or ( require( "Genome" ).Sprite.renderMode() == "ascii" and "dissolve" or "fade" )
  if not TRANSITIONS[kind] then error( "Scene.go: unknown transition '"..tostring( kind ).."' (fade, wipe, dissolve, none)" ) end
  local dur = opts.duration or 0.6
  if kind == "none" or dur <= 0 then
    swap( p.name, opts )
    return
  end
  local half = math.max( 1, math.floor( dur * FPS / 2 + 0.5 ) )
  S.trans = { name = p.name, opts = opts, kind = TRANSITIONS[kind], phase = "out", f = 0, n = half }
  -- Music: fade out unless the next scene plays the same song (or opts.music = "keep" / "cut").
  local nextMusic = sceneTable( p.name ).music
  local Sound = require( "Sound" )
  local musicMode = opts.music or ( nextMusic and nextMusic.transition ) or "crossfade"
  S.trans.fadeMusic = musicMode == "crossfade" and Sound.song and
                      not ( nextMusic and nextMusic.song == Sound.song )
  if S.trans.fadeMusic then ffi.C.musicFade( 0, half * 1000 / FPS ) end
end

local function frame()
  Scene.frameNo = Scene.frameNo + 1
  flushRemovals()
  local W = package.loaded["World3D"]
  if W and type( W ) == "table" and W.dying and #W.dying > 0 then  -- bodies removed this frame: free their ids
    for _, id in ipairs( W.dying ) do ffi.C.despawnEntity( id ) end
    W.dying = {}
  end
  if S.cycle then S.cycle() end
  if not S.trans and S.pending then
    local p = S.pending
    S.pending = false
    startTransition( p )
  end
  local t = S.trans
  if not t then return end
  t.f = t.f + 1
  if t.phase == "out" then
    ffi.C.setTransition( t.kind, t.f / t.n )
    if t.f >= t.n then
      swap( t.name, t.opts )
      if t.fadeMusic then ffi.C.musicFade( 1, t.n * 1000 / FPS ) end
      t.phase, t.f = "in", 0
      ffi.C.setTransition( t.kind, 1 )
    end
  else
    ffi.C.setTransition( t.kind, 1 - t.f / t.n )
    if t.f >= t.n then
      ffi.C.setTransition( 0, 0 )
      S.trans = false
    end
  end
end

sceneHooks = { frame = frame, inputFrozen = function() return S.trans ~= false end }

-- BICYCLE_SCENE_CYCLE=<n>[:<scene a>:<scene b>:<frames>]: headless lifecycle test (tests/scene_lifecycle_test.sh).
-- Goes back and forth between two scenes n times, one change every <frames> frames, then quits.
do
  local spec = os.getenv( "BICYCLE_SCENE_CYCLE" )
  if spec and spec ~= "" then
    local n, a, b, every = spec:match( "^(%d+):?([%w_]*):?([%w_]*):?(%d*)$" )
    n, every = tonumber( n ) or 0, tonumber( every ) or 24
    a = ( a ~= "" and a ) or "intro"
    b = ( b ~= "" and b ) or "house"
    local done = 0
    S.cycle = function()
      if Scene.busy() or Scene.frameNo < 20 or ( Scene.frameNo - Scene.enteredFrame ) < every then return end
      if done >= n then
        while ffi.C.topWindowId() ~= 0 do ffi.C.popWindow() end
        return
      end
      done = done + 1
      Scene.go( Scene.file == a and b or a, { duration = 0.2, transition = os.getenv( "BICYCLE_SCENE_CYCLE_TRANSITION" ) } )
    end
  end
end

return Scene
