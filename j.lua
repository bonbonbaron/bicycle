ffi = require("ffi")

ffi.cdef([[
  typedef enum {
      // Letters (HID 0x04–0x1D)  <-- these will be treated as case-insensitive
      A ,
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
    int triggeredCallbackRef;
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

  void pushDialogue( const char* text, int x, int y, int w, int h );
  void pushTextMenu( int x, int y, int w, int h );
  void addItem( const char* text, int cbRef );
]])

rects = ffi.C.World_getRects();

r2 = rects.arr[2]
r2.pos.x = r2.pos.x + 5
ffi.C.pushTextMenu( 5, 5, 30, 5 )
-- TODO add a bicycle.lua wrapper for calling this so we dont' have to worry
function a(name) ffi.C.pushDialogue( "hi, i'm an "..name, 10, 10, 20, 3 ) end
menuFuncs = {
  function() a( "apple" ) end,
  function() a( "orange" ) end
}
ffi.C.addItem( "Apple", 1 )
ffi.C.addItem( "Orange", 2 )

local bridge
function initBridge( bridgePtr )
  bridge = ffi.cast("Bridge*", bridgePtr)
end

function getUpdates( bridgePtr )
  -- Collisions
  -- Time-outs
  -- Inputs
  if bridge.input.triggeredCallbackRef ~= 0 then
    func = menuFuncs[bridge.input.triggeredCallbackRef]
    if func then
      func()
    end
  end
end
