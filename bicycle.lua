ffi = require("ffi")
require("table")
require("io")

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

  typedef struct {
    Rect rect;
    unsigned type;
  } CollRect;

  typedef struct {
    Rect* arr;
    unsigned len;
  } WorldRectArr;

  typedef struct {
    Rect* arr;
    unsigned len;
  } WorldRectArr;

  typedef struct {
    Velocity* arr;
    unsigned len;
  } WorldVelArr;

  WorldRectArr World_getRects();

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

]])

Window = {}
Window = {}
Window.__index = Window
Window.__newindex = function() error( "You can't create new attributes for Window." ) end

function Window.pop(id)
  focusOn( ffi.C.popWindow() )  -- focuses on table before this one
  unregister(id)
end

function Window.push(id, entity)
  -- print("id: "..tostring(id).." entity: "..tostring(entity))
  register(id, entity)
  focusOn(id)
end

---------------------------------------
Dialogue = {}
Dialogue.__index = Dialogue
Dialogue.__newindex = function() error( "You can't create new attributes for Dialogue." ) end
Dialogue.new = function( text, x, y, w, h )
  local id = ffi.C.pushDialogue( text, x, y, w, h )
  local instance = { id=id, x=x, y=y, w=w, h=h, cbs = {} }
  setmetatable( instance, Dialogue )
  setmetatable( Dialogue, Window )
  Window.push(id, instance)
  return instance
end

function Dialogue:onInput( input )
  if input == ffi.C.Space or input == ffi.C.B then
    Window.pop(self.id)
  end
end

------------------------------------
Menu = {}
Menu.__index = Menu
Menu.__newindex = function() error( "You can't create new attributes for Menu." ) end
Menu.new = function( x, y, w, h )
  local x = x or 0 
  local y = y or 0
  local w = w or 10
  local h = h or 10
  local id = ffi.C.pushTextMenu( x, y, w, h )
  local instance = { id=id, x=x, y=y, w=w, h=h, cbs = {} }
  setmetatable( instance, Menu )
  setmetatable( Menu, Window )
  Window.push(id, instance)
  return instance
end

function Menu:add( str, cb )
  if type(cb) == "function" then
    ffi.C.addItem( str )
    table.insert( self.cbs, cb )  -- index of cursor corresponds to selection
  end
end

-- TODO function Menu::remove(), good for item menus in RPGs

function Menu:onInput( input )
  -- print("menu hears ya")
  abc = io.read()
  if input == ffi.C.Space then
    selIdx = ffi.C.getSelection()
    self.cbs[selIdx+1]()
  elseif input == ffi.C.B then
    self.pop(self.id)
  else
    -- print("do nothing")
  end
end

-------------------------------------

-- TODO load entities from genomes

local entities = {}  -- maps entity ID to callbackID-to-callback maps
local focus = 0

function register( entityId, entity )
  print("regitering "..entityId)
  entities[entityId] = entity -- TODO check whether entity ID is already used in table later
end

function unregister( id )
  if not id then
    error("trying to unregsiter "..id, 5)
  end
  entities[id] = nil
end

function focusOn( entity )
  --print("focusing on ".. tostring(entity))
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
