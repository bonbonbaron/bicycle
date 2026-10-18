ffi = require("ffi")
require("table")
require("io")
require("debug")

ffi.cdef([[
  static const unsigned FIXEDPT_DEC_BITS = 8;
  typedef enum {
      // Letters (HID 0x04–0x1D)  <-- these will be treated as case-insensitive
      NOTHING = 0,
      A = 1,
      B ,
      C ,
      D ,
      E ,
      F ,
      G ,
      H ,
      I ,
      J ,
      K ,
      L ,
      M ,
      N ,
      O ,
      P ,
      Q ,
      R ,
      S ,
      T ,
      U ,
      V ,
      W ,
      X ,
      Y ,
      Z ,

      // Numbers (HID 0x1E–0x27)
      Key1 ,
      Key2 ,
      Key3 ,
      Key4 ,
      Key5 ,
      Key6 ,
      Key7 ,
      Key8 ,
      Key9 ,
      Key0 ,

      // Common punctuation / symbols
      Enter     ,
      Escape    ,
      Tab       ,
      Space     ,

      // Modifiers
      LeftCtrl ,
      LeftShift ,
      LeftAlt   ,     // Left Option on macOS
      LeftMeta  ,     // Left Win / Left Cmd
      RightCtrl ,
      RightShift,
      RightAlt  ,     // Right Option / AltGr
      RightMeta ,     // Right Win / Right Cmd

      MouseLeft ,
      MouseRight,
      MouseMiddle,
      Look      ,     // mouse look: onInput( self, Look, true, dx, dy ) once per frame when the mouse moved

      COUNT     // ← use this to know how many we defined (not a real key)
  } LogicalKey;

  typedef struct {  // same field order as include/m/Size.h
    unsigned w;
    unsigned h;
    unsigned d;
  } Size;

  typedef struct {
    int x;
    int y;
    int z;
  } Position;

  typedef Position Velocity;

  typedef struct {
    Position pos;
    Size size;
  } Box;

  typedef enum {
    BLACK	  ,
    RED	    ,
    GREEN	  ,
    YELLOW  ,
    BLUE	  ,
    MAGENTA ,
    CYAN	  ,
    WHITE   ,
  } Color;

  typedef struct {
    char* symbol;
    Color color;
    bool visible;
  } Image;

  typedef struct {
    Box box;
    unsigned type;
  } CollBox;

  typedef struct {
    unsigned duration;
    Box srcBox;
    CollBox collisionBox;
  } AnimFrame;

  typedef enum { LOOP, ONE_SHOT, PINGPONG } AnimType;

  typedef struct {
    unsigned nFrames;
    AnimType type;
    AnimFrame* frames;
  } AnimStrip;

  // world arrays

  typedef struct {
    Box* arr;
    unsigned len;
  } WorldBoxArr;

  typedef struct {
    CollBox* arr;
    unsigned len;
  } WorldCollBoxArr;

  typedef struct {
    Velocity* arr;
    unsigned len;
  } WorldImgArr;

  typedef struct {
    AnimStrip* arr;
    unsigned len;
  } WorldAnimArr;

  typedef struct {
    Velocity* arr;
    unsigned len;
  } WorldVelArr;

  // world array getters

  WorldBoxArr World_getBoxes();
  WorldCollBoxArr World_getCollBoxes();
  WorldImgArr  World_getImages();
  WorldAnimArr World_getAnims();

  typedef unsigned Entity;

  typedef enum { TO_BRIDGE, TO_ANIMATION, TO_RENDERING, TO_MOTION } TimeoutAddr;  // Notice how this only addresses outputs. Cool.

  typedef struct {
    Entity entity;
    unsigned id;      // An entity can have multiple timers running simulatneously. ID
    unsigned type;    // I *think* this corresponds to the activity we want this to spark in recipient.
    TimeoutAddr addr;  // Subtimers don't kick off new activity groups.
  } Timeout;

  typedef struct {
    Entity lhs;  // think of this is "self"
    Entity rhs;  // whom you collided with
    unsigned type; // their collision type
  } Collision;

	typedef struct {
		Timeout* arr;
		unsigned len;  // current number of elements 
		unsigned cap;
  } TimeoutArr;

	typedef struct {
		Collision* arr;
		unsigned len;  // current number of elements 
		unsigned cap;
  } CollisionArr;

  typedef struct {
    uint8_t lastPressed;  // C++'s LogicalKey is a uint8_t enum
    int millisSinceLast;
    // int currKeysPressed;
    // int deltaKeysPressed;
  } Input;

  typedef struct {
    uint8_t arr[32];
    uint8_t down[32];  // 1 pressed, 0 released
    unsigned len;
  } KeyQueue;

	typedef struct {
		TimeoutArr timeouts;
		CollisionArr collisions;
		CollisionArr uncollisions;
		KeyQueue keys;  // every key since the last getUpdates(), in order
		float lookDx, lookDy;  // mouse look since the last getUpdates()
		Input input; // Input is just a scalar.  // TODO should this be initialized?
	} Bridge;

  typedef enum { START, STOP, PAUSE, UNPAUSE } Action;
  typedef enum { RECT, COLLRECT, IMAGE } Component;
  typedef enum { SYS_TIMER, SYS_MOTION } System;
  Entity pushDialogue( const char* text, int x, int y, int w, int h );
  bool isDialogueDone();
  int getScreenWidth();
  int getScreenHeight();
  Entity pushTextMenu( int x, int y, int w, int h );
  void addItem( const char* text );
  unsigned getSelection();
  Entity pushScene();
  Entity popWindow();

  // Background layer types (C++'s LayerType): 0 FIXED, 1 GLUED, 2 PARALLAX, 3 LOOP, 4 AUTOLOOP. See Genome.lua's Sprite.BG_MODES.
  Entity newEntity();
  Entity delEntity();
  void addBgLayer( const char* bgStr );
  void addBgLayerEx( const char* bgStr, int type, int parallaxPct, int parallaxPctY, int vx, int vy, int x, int y );
  void addFgEntity( Entity entity );
  void newImage( Entity entity, const char* imgStr );
  int renderMode();
  bool setModel( Entity entity, const char* bmeshPath );
  void setModelAction( Entity entity, const char* action );
  bool addStaticModel( const char* bmeshPath );
  bool newPixelImage( Entity entity, const char* paths );
  bool newPixelImagePal( Entity entity, const char* paths, const char* palette );
  bool addBgLayerPixels( const char* tilesetPath, const int* tiles, int nw, int nh, int tileW, int tileH,
                         int type, int parallaxPct, int parallaxPctY, int vx, int vy, int x, int y );
  void setPos( Entity entity, Position pos );
  void addCollider( Entity entity, unsigned layer, unsigned type, int x, int y, int w, int h );
  void setVelocity( Entity entity, int vx, int vy );
  void motionLinear( Entity entity, float speed, bool pingPong );
  void motionWaypoint( Entity entity, float x, float y );
  void motionOrbit( Entity entity, Entity center, float cx, float cy, float rx, float ry, float degPerSec, float phaseDeg );
  void motionCombo( Entity entity, float rx, float ry, float degPerSec, float phaseDeg, float speed, bool pingPong );
  void motionFollow( Entity entity, Entity target, float offX, float offY, float rate, float bobRows, float bobHz, bool mirror, float deadX, float deadY );
  Entity cameraEntity();
  void motionStop( Entity entity );
  int motionType( Entity entity );
  void newAnimStrip( Entity entity, uint8_t stripId, bool repeat );
  bool isReleaseBuild();
  void addAnimFrame( Entity entity, uint8_t stripId, unsigned durationMs, int x, int y, int w, int h );
  bool setAnim( Entity entity, uint8_t stripId );
  void stopAnim( Entity entity );
  unsigned getAnimFrameIdx( Entity entity );
  void focusCamOn( Entity entity );
  void sys(const Action action, const System system, Entity entity);

  // MIDI PLAYER
  void playSound( const int chan, const int key, const int vel );
  void stopSound( const int chan );
  void stopSoundFX();
  void playSoundFX( const int key, const int vel );
  void playSong();
  void stopSong();
  void pauseSong();
  void unpauseSong();
  bool loadMidi( const char* midiFilepath );
  int loadSoundfont( const char* sfFilepath );
  bool selectProgram( const int chan, const int sfontId, const int bank, const int program );
  int getSfxChannel();
  int addHudBar( const char* label, int max, int len, int x, int y );
  void setHudBar( int id, int val );
  int addHudText( const char* text, int x, int y );
  void setHudText( int id, const char* text );
  float frameRate();
  bool setHudBarSprites( int id, const char* framePath, const char* fillPath );
  void setHidden( Entity entity, bool hidden );

  // Scene changes (Scene.goto in scripts/Scene.lua)
  void despawnEntity( Entity entity );
  void sceneScope( bool on );
  long long sceneUnloaded();
  void sceneStats( const char* event, const char* sceneName, long long leaked );
  void setTransition( int kind, float t );
  void musicFade( float level, unsigned ms );
  void motionSnap( Entity entity );
  void attachEntity( Entity entity, unsigned layerIdx );
  bool topIsScene();
  Entity topWindowId();

  Entity pop() {
    auto& wm = WindowManager::getInstance();
    return wm.pop();
  }

  typedef unsigned TimerId;
  TimerId createTimer( const unsigned timeMs, Entity entity, const unsigned timeoutType, const bool repeat, const TimeoutAddr addr );
  void stopTimer( const TimerId timerId );

  int rollDice( const int min, const int max );

  const char* assetRead( const char* path, size_t* len );
  bool assetExists( const char* path );
  bool assetPacked();
  const char* assetList( const char* dir );
]])

-- Game assets come from the asset pack in release builds (-DBICYCLE_RELEASE_ASSETS=ON: <game>/assets.bpak; -DBICYCLE_RELEASE=ON:
-- embedded), inflated by the engine; dev builds always read loose files, even if an old assets.bpak is lying around.
Asset = {}
-- Outside the engine (e.g. plain luajit running a headless test) there's no ffi.C.assetRead: use files.
local inEngine = pcall( function() return ffi.C.assetRead end )
function Asset.read( path )
  if not inEngine then
    local f = io.open( path, "rb" )
    if not f then return nil end
    local s = f:read( "*a" )
    f:close()
    return s
  end
  local len = ffi.new( "size_t[1]" )
  local p = ffi.C.assetRead( path, len )
  if p == nil then return nil end
  return ffi.string( p, len[0] )
end
function Asset.exists( path )
  if not inEngine then
    local f = io.open( path, "rb" )
    if f then f:close() end
    return f ~= nil
  end
  return ffi.C.assetExists( path )
end
-- Runs a Lua asset (a genome table, tilemap, ...) and returns its result.
function Asset.dofile( path )
  local code = Asset.read( path )
  if not code then return nil, "no asset '"..path.."'" end
  local chunk, err = loadstring( code, "@"..path )
  if not chunk then return nil, err end
  return chunk()
end
-- Paths of the files in a folder: from the pack, or nil when there's no pack (list the folder yourself).
function Asset.list( dir )
  if not inEngine or not ffi.C.assetPacked() then return nil end
  local out = {}
  for line in ffi.string( ffi.C.assetList( dir ) ):gmatch( "[^\n]+" ) do table.insert( out, line ) end
  return out
end

-- Set BICYCLE_DEBUG_LOG=<file> to have games log events (dbglog) for testing; otherwise a no-op.
local debugLogFile = os.getenv( "BICYCLE_DEBUG_LOG" ) and io.open( os.getenv( "BICYCLE_DEBUG_LOG" ), "w" )
function dbglog( fmt, ... )
  if debugLogFile then
    debugLogFile:write( string.format( "%.3f ", os.clock() ), string.format( fmt, ... ), "\n" )
    debugLogFile:flush()
  end
end

function checkType( arg, expType )
  if type(arg) ~= expType then 
    error( debug.traceback().."\n\nExpected a "..expType..", got a "..type(arg) )
  end
end

gameName = ""

local entities = {}  -- maps entity ID to callbackID-to-callback maps
sceneHooks = nil  -- set by scripts/Scene.lua: { frame = fn, inputFrozen = fn }
local focus = 0

function register( entityId, entity )
  entities[entityId] = entity -- TODO check whether entity ID is already used in table later
end

-- The table registered for an entity id (a genome instance: e.g. the hero a coin was touched by), or nil.
function entityOf( id )
  return entities[id]
end

function unregister( id )
  if not id then
    error( debug.traceback( "unregister: no entity id" ), 2 )
  end
  entities[id] = nil
  listeners[id] = nil
end

-- Input goes to the focused entity, and also to listeners: entities that asked for every input event (a camera
-- genome orbiting on mouse look while the hero has the focus). Genomes listen with the field listen = true.
listeners = {}
function listenInput( id, on )
  listeners[id] = ( on ~= false ) and true or nil
end

-- Engine per-frame work that turns state into events (World3D: onLand, onFallOut). Not for game code: games react
-- to events (onInput, onTimer, onCollision, ...).
engineFrameHooks = {}

-- Tracking, for any entity and the camera alike (Kinematics FOLLOW_LERP; the camera is just a kinematic body):
--   opts = { mode = "lerp" | "hard", rate = 8, offset = { x, y }, deadzone = { x, y }, mirror = false,
--            bob = 0, bobHz = 1 }
-- mode "hard" snaps to target + offset every frame; "lerp" eases: pos += (goal - pos) * (1 - exp(-rate * dt)).
-- deadzone (cells each side) lets the target roam that far before the follower moves. Call again to switch modes.
trackOpts = {}  -- follower -> { target, opts } as last asked (scene changes carry the hero's followers over)
function track( follower, target, opts )
  opts = opts or {}
  trackOpts[follower] = { target = target, opts = opts }
  local off, dz = opts.offset or { 0, 0 }, opts.deadzone or { 0, 0 }
  local rate = opts.mode == "hard" and 0 or ( opts.rate or opts.smoothing or 8 )
  ffi.C.motionFollow( follower, target, off[1], off[2], rate, opts.bob or 0, opts.bobHz or 1,
                      opts.mirror and true or false, dz[1], dz[2] )
end

-- The scene camera. By default it hard-tracks the focused entity with a dead zone (the scene's margins);
-- Camera.track( entity, opts ) takes the same opts as track(), e.g.
--   Camera.track( hero.id, { mode = "lerp", rate = 6, deadzone = { 8, 3 } } )
--   Camera.track( hero.id, { mode = "hard" } )
Camera = {}
function Camera.id() return ffi.C.cameraEntity() end
-- The camera's last tracking is remembered (Camera.target, Camera.opts) and re-applied in the next scene.
function Camera.track( target, opts )
  Camera.target, Camera.opts = target, opts
  track( ffi.C.cameraEntity(), target, opts )
end

-- Frames per second, smoothed over about half a second (0 for the first half second), e.g. for a HUD label.
function fps() return tonumber( ffi.C.frameRate() ) end

function focusOn( entity )
  focus = entity
end

function getFocus()
  return focus
end

--TODO write wrappers for menu and dialogue that 1) don't require dimensions and 2) auto-focus on them, 3) send moveCursorUp/Down()
--TODO write wrappers for system calls

function move( entity, vel )
  checkType( entity, "number" )
  checkType( vel, "table" )
  -- TODO write a motion config wrapper in bicycle.cpp.
  -- TODO write a Motion table that defaults values and allows you to easily define a motion in Lua. Then you can handle all the nasty configuration stuff here.
  if vel.x and vel.y then
    sys( START, SYS_MOTION, entity )
  end
end

-------------------------------------

local bridge
function initBridge( bridgePtr )
  bridge = ffi.cast("Bridge*", bridgePtr)
end

-- Drops queued uncollisions addressed to entities that no longer exist (despawned by a scene change before Lua
-- read them; their ids may already belong to the next scene's entities).
function dropUncollisionsFor( isGone )
  if not bridge then return end
  local u, n = bridge.uncollisions, 0
  for i = 0, u.len - 1 do
    if not isGone( u.arr[i].lhs ) then
      u.arr[n] = u.arr[i]
      n = n + 1
    end
  end
  u.len = n
end

function getUpdates()
  -- Collisions
  if bridge.collisions.len > 0 then
    for i = 0, bridge.collisions.len - 1 do
      coll = bridge.collisions.arr[i]
      if coll.lhs ~= 0 then  -- 0: voided (Trigger::forgetEntity), its entity went away
      entity = entities[coll.lhs]
      if not entity then error("Collision recorded for nonexistent entity "..tostring(coll.lhs)..".") end
      if entity.onCollision then entity:onCollision( coll.rhs, coll.type ) end
      end
    end
    bridge.collisions.len = 0  -- consumed; C++ appends from len
  end
  -- Uncollisions
  if bridge.uncollisions.len > 0 then
    for i = 0, bridge.uncollisions.len - 1 do
      coll = bridge.uncollisions.arr[i]
      if coll.lhs ~= 0 then
      entity = entities[coll.lhs]
      if not entity then error("Un-collision recorded for nonexistent entity "..tostring(coll.lhs)..".") end
      if entity.onUncollision then entity:onUncollision( coll.rhs, coll.type ) end
      end
    end
    bridge.uncollisions.len = 0  -- consumed; C++ appends from len
  end
  -- Time-outs
  if bridge.timeouts.len > 0 then
    for i = 0, bridge.timeouts.len - 1 do
      timeout = bridge.timeouts.arr[i]
      if timeout.entity ~= 0 then
      entity = entities[timeout.entity]
      if not entity then error("Timeout recorded for nonexistent entity "..tostring(timeout.entity)..".") end
      -- onTimer (genomes' timer gene) or the older name onTimeout
      local cb = entity.onTimer or entity.onTimeout
      if cb then cb( entity, timeout.id, timeout.type ) end
      end
    end
    bridge.timeouts.len = 0  -- consumed; C++ appends from len
  end
  -- Inputs (every key since last frame, in order)
  if bridge.keys.len > 0 and sceneHooks and sceneHooks.inputFrozen() then
    -- Mid scene transition: presses are dropped, releases still go through (a key held through a door is let go).
    local n = 0
    for i = 0, bridge.keys.len - 1 do
      if bridge.keys.down[i] == 0 then
        bridge.keys.arr[n], bridge.keys.down[n] = bridge.keys.arr[i], 0
        n = n + 1
      end
    end
    bridge.keys.len = n
    bridge.input.lastPressed = ffi.C.NOTHING
  end
  if ( bridge.lookDx ~= 0 or bridge.lookDy ~= 0 ) and sceneHooks and sceneHooks.inputFrozen() then
    bridge.lookDx, bridge.lookDy = 0, 0
  end
  if bridge.keys.len > 0 or bridge.lookDx ~= 0 or bridge.lookDy ~= 0 then
    entity = entities[focus]
    if not entity then error("We're focused on nonexistent entity "..tostring(focus)..".") end
    local function deliver( key, pressed, dx, dy )
      if inputLog then
        inputLog:write( string.format( "input %d %s\n", key, pressed and "down" or "up" ) )
        inputLog:flush()
      end
      if entity.onInput then entity:onInput( key, pressed, dx, dy ) end
      for id in pairs( listeners ) do
        local l = entities[id]
        if l and id ~= focus and l.onInput then l:onInput( key, pressed, dx, dy ) end
      end
    end
    for i = 0, bridge.keys.len - 1 do
      deliver( bridge.keys.arr[i], bridge.keys.down[i] ~= 0 )
    end
    bridge.keys.len = 0
    bridge.input.lastPressed = ffi.C.NOTHING
    -- Mouse look: once per frame, the motion since the last one (pixels; the gamepad's right stick adds to it).
    local dx, dy = bridge.lookDx, bridge.lookDy
    if dx ~= 0 or dy ~= 0 then
      bridge.lookDx, bridge.lookDy = 0, 0
      deliver( ffi.C.Look, true, dx, dy )
    end
  end
  for _, hook in ipairs( engineFrameHooks ) do hook( frameDt ) end
  -- Real-time games: a per-frame hook after the events (Constants::MILLISECONDS_PER_FRAME apart).
  if onFrame then onFrame( frameDt ) end
  -- The frame's end, after every callback: queued scene changes and transitions run here (scripts/Scene.lua).
  if sceneHooks then sceneHooks.frame() end

end

frameDt = 1 / 30
-- BICYCLE_INPUT_LOG=1 prints every input event ("input <key> down|up") to stderr; =<path> appends them to a file.
inputLog = os.getenv( "BICYCLE_INPUT_LOG" )
if inputLog and inputLog ~= "1" and inputLog ~= "" then inputLog = io.open( inputLog, "a" ) or nil
elseif inputLog then inputLog = io.stderr end
