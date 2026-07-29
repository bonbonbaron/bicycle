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

      COUNT     // ← use this to know how many we defined (not a real key)
  } LogicalKey;

  typedef struct { 
    unsigned h;
    unsigned w;
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
    LogicalKey lastPressed;
    // int millisSinceLast;  // i don't like making input dependent on Timer.
    // int currKeysPressed;
    // int deltaKeysPressed;
  } Input;

	typedef struct {
		TimeoutArr timeouts;
		CollisionArr collisions;
		CollisionArr uncollisions;
		Input input; // Input is just a scalar.  // TODO should this be initialized?
	} Bridge;

  typedef enum { START, STOP, PAUSE, UNPAUSE } Action;
  typedef enum { RECT, COLLRECT, IMAGE } Component;
  typedef enum { SYS_TIMER, SYS_MOTION } System;
  Entity pushDialogue( const char* text, int x, int y, int w, int h );
  Entity pushTextMenu( int x, int y, int w, int h );
  void addItem( const char* text );
  unsigned getSelection();
  Entity pushScene();
  Entity popWindow();

  enum LayerType { FIXED, GLUED, PARALLAX, AUTOLOOP };
  Entity newEntity();
  Entity delEntity();
  void addBgLayer( const char* bgStr );
  void addFgEntity( Entity entity );
  void newImage( Entity entity, const char* imgStr );
  void setPos( Entity entity, Position pos );
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
  void loadMidi( const char* midiFilepath );
  void loadSoundfont( const char* sfFilepath );

  Entity pop() {
    auto& wm = WindowManager::getInstance();
    return wm.pop();
  }

  typedef unsigned TimerId;
  TimerId createTimer( const unsigned timeMs, Entity entity, const unsigned timeoutType, const bool repeat, const TimeoutAddr addr );

  ]])

  function checkType( arg, expType )
    if type(arg) ~= expType then 
      error( debug.traceback().."\n\nExpected a "..expType..", got a "..type(arg) )
    end
  end

  gameName = ""

  local entities = {}  -- maps entity ID to callbackID-to-callback maps
  local focus = 0

  function register( entityId, entity )
    entities[entityId] = entity -- TODO check whether entity ID is already used in table later
  end

  function unregister( id )
    if not id then
      error("trying to unregsiter "..id, 5)
    end
    entities[id] = nil
  end

  function focusOn( entity )
    focus = entity
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

  function getUpdates()
    -- Collisions
    if bridge.collisions.len > 0 then
      for i = 0, bridge.collisions.len do
        coll = bridge.collisions.arr[i]
        entity = entities[coll.lhs]
        if not entity then error("Collision recorded for nonexistent entity "..tostring(coll.lhs)..".") end
        entity:onCollision( coll.rhs, coll.type )
      end
    end
    -- Uncollisions
    if bridge.uncollisions.len > 0 then
      for i = 0, bridge.uncollisions.len do
        coll = bridge.uncollisions.arr[i]
        entity = entities[coll.lhs]
        if not entity then error("Un-collision recorded for nonexistent entity "..tostring(coll.lhs)..".") end
        entity:onCollision( coll.rhs, coll.type )
      end
    end
    -- Time-outs
    -- Inputs
    if bridge.input.lastPressed ~= ffi.C.NOTHING then
      entity = entities[focus]
      if not entity then error("We're focused on nonexistent entity "..tostring(focus)..".") end
      if entity and bridge.input.lastPressed then
        entity:onInput( bridge.input.lastPressed )
        bridge.input.lastPressed = ffi.C.NOTHING
      end
    end
  end
