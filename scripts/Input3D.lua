-- Real-time input for 3D games (include/c/InputState.h): held keys, mouse look, gamepad, mapped to actions.
--   local Input = require("Input3D")
--   Input.bind( "jump", { "space", "pad:a" } )
--   Input.bindAxis( "moveX", { neg = { "a" }, pos = { "d" }, pad = "leftx" } )
--   if Input.pressed( "jump" ) then ... end ; local x = Input.axis( "moveX" )
--   local dx, dy = Input.look()   -- mouse (pixels) + right stick (scaled), this frame
local ffi = require("ffi")
ffi.cdef([[
  bool inputHeld( int key );
  bool inputPressed( int key );
  float inputMouseDx();
  float inputMouseDy();
  float inputWheel();
  bool inputMouseButton( int b );
  bool inputMousePressed( int b );
  float inputPadAxis( int a );
  bool inputPadButton( int b );
  bool inputPadPressed( int b );
  bool inputPadConnected();
  void inputSetMouseCapture( bool on );
  bool inputMouseCaptured();
  void inputEnableGamepad();
]])
local C = ffi.C
local Input = { deadzone = 0.18, stickLook = 600 }  -- stickLook: right stick full tilt = this many "pixels"/s

-- LogicalKey values (include/c/Input.h): A..Z = 1..26, 1..9 = 27..35, 0 = 36, enter 37, escape 38, tab 39, space 40,
-- lctrl 41, lshift 42.
local KEYS = { enter = 37, escape = 38, tab = 39, space = 40, ctrl = 41, shift = 42 }
for i = 0, 25 do KEYS[string.char( 97 + i )] = i + 1 end
for i = 1, 9 do KEYS[tostring( i )] = 26 + i end
KEYS["0"] = 36
-- SDL_GamepadButton / SDL_GamepadAxis
local PAD = { a = 0, b = 1, x = 2, y = 3, back = 4, guide = 5, start = 6, ls = 7, rs = 8, lb = 9, rb = 10,
              up = 11, down = 12, left = 13, right = 14 }
local AXES = { leftx = 0, lefty = 1, rightx = 2, righty = 3, lt = 4, rt = 5 }
local MOUSE = { ["mouse:left"] = 0, ["mouse:right"] = 1, ["mouse:middle"] = 2 }

local actions, axes = {}, {}
function Input.bind( action, inputs ) actions[action] = inputs end
function Input.bindAxis( name, spec ) axes[name] = spec end

local function one( name, edge )
  if MOUSE[name] then return edge and C.inputMousePressed( MOUSE[name] ) or ( not edge and C.inputMouseButton( MOUSE[name] ) ) end
  local pad = name:match( "^pad:(.+)$" )
  if pad then
    if AXES[pad] then return C.inputPadAxis( AXES[pad] ) > 0.5 end
    return edge and C.inputPadPressed( PAD[pad] ) or ( not edge and C.inputPadButton( PAD[pad] ) )
  end
  local k = KEYS[name] or error( "Input3D: unknown key "..name )
  if edge then return C.inputPressed( k ) end
  return C.inputHeld( k )
end
function Input.down( action )
  for _, n in ipairs( actions[action] or {} ) do if one( n, false ) then return true end end
  return false
end
function Input.pressed( action )
  for _, n in ipairs( actions[action] or {} ) do if one( n, true ) then return true end end
  return false
end
function Input.axis( name )
  local s = axes[name]
  if not s then return 0 end
  local v = 0
  for _, n in ipairs( s.neg or {} ) do if one( n, false ) then v = v - 1 end end
  for _, n in ipairs( s.pos or {} ) do if one( n, false ) then v = v + 1 end end
  if s.pad then
    local a = C.inputPadAxis( AXES[s.pad] ) * ( s.padScale or 1 )
    if math.abs( a ) > Input.deadzone then v = v + a end
  end
  return math.max( -1, math.min( 1, v ) )
end
-- Look deltas this frame: mouse pixels plus the right stick (as pixels at stickLook per second).
function Input.look( dt )
  local dx, dy = C.inputMouseDx(), C.inputMouseDy()
  local rx, ry = C.inputPadAxis( 2 ), C.inputPadAxis( 3 )
  if math.abs( rx ) > Input.deadzone then dx = dx + rx * Input.stickLook * ( dt or 1 / 30 ) end
  if math.abs( ry ) > Input.deadzone then dy = dy + ry * Input.stickLook * ( dt or 1 / 30 ) end
  return dx, dy
end
function Input.captureMouse( on ) C.inputSetMouseCapture( on ) end
function Input.enableGamepad() C.inputEnableGamepad() end
function Input.padConnected() return C.inputPadConnected() end

return Input
