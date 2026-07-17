ffi = require("ffi")

ffi.cdef([[
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


  void pushDialogue( const char* text, int x, int y, int w, int h );
  void pushTextMenu( const char* text, int x, int y, int w, int h );
]])

rects = ffi.C.World_getRects();

r2 = rects.arr[2]
-- print("lua: r2 x is "..r2.pos.x)
r2.pos.x = r2.pos.x + 5
-- print("lua: r2 x is "..r2.pos.x)
ffi.C.pushDialogue( "lua: r2 x is "..r2.pos.x, 5, 5, 30, 5 );
