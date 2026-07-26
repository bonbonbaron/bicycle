ffi = require("ffi")
require("table")
require("io")
require("debug")

ffi.cdef([[
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
  } Rect;

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
    Rect rect;
    unsigned type;
  } CollRect;

  typedef struct {
    unsigned duration;
    Rect srcRect;
    CollRect collisionRect;
  } AnimFrame;

  typedef enum { LOOP, ONE_SHOT, PINGPONG } AnimType;

  typedef struct {
    unsigned nFrames;
    AnimType type;
    AnimFrame* frames;
  } AnimStrip;

  // world arrays

  typedef struct {
    Rect* arr;
    unsigned len;
  } WorldRectArr;

  typedef struct {
    CollRect* arr;
    unsigned len;
  } WorldCollRectArr;

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

  WorldRectArr World_getRects();
  WorldCollRectArr World_getCollRects();
  WorldImgArr  World_getImages();
  WorldVelArr World_getVels();
  WorldRectArr World_getVels();
  WorldAnimArr World_getAnims();

  typedef unsigned Entity;

  typedef struct {
    Entity entity;
    unsigned id;      // An entity can have multiple timers running simulatneously. ID
    unsigned type;    // I *think* this corresponds to the activity we want this to spark in recipient.
    bool isSubtimer;  // Subtimers don't kick off new activity groups.
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
    Entity focus;
    LogicalKey lastPressed;
    // int millisSinceLast;  // i don't like making input dependent on Timer.
    // int currKeysPressed;
    // int deltaKeysPressed;
  } Input;

	typedef struct {
		TimeoutArr timeouts;
		CollisionArr collisions;
		Input input; // Input is just a scalar.  // TODO should this be initialized?
	} Bridge;

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

-------------------------------------

local bridge
function initBridge( bridgePtr )
  bridge = ffi.cast("Bridge*", bridgePtr)
end

function getUpdates()
  -- Collisions
  -- Time-outs
  -- Inputs
  if bridge.input.lastPressed ~= ffi.C.NOTHING then
    entity = entities[focus]
    if entity and bridge.input.lastPressed then
      entity:onInput( bridge.input.lastPressed )
      bridge.input.lastPressed = ffi.C.NOTHING
    end
  end
end
