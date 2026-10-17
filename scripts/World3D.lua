-- The true-3D layer from Lua (include/c/Physics3D.h, src/c/World3DApi.cpp). Meters, seconds, degrees, Y up;
-- yaw 0 looks down -Z. An entity's position is its feet. 2D games don't need this module.
--
--   local W = require("World3D")
--   W.level( "courtyard" )                               -- genome/level/courtyard.lua: draw + collide its static meshes
--   local hero = W.Body.new{ kind = "character", x = 0, y = 1, z = 0, model = "hero3d", modelYaw = -90 }
--   W.Camera.orbit( hero.id, { distance = 6, height = 1.4, rate = 10 } )
--   hero:move( right, fwd ) (camera-relative, on a key press/release) or hero:wish( vx, vz ); hero:jump()
--   local hit = W.raycast( eye, dir, 100, hero.id )      -- hitscan: { x, y, z, nx, ny, nz, t, entity }
local ffi = require("ffi")
ffi.cdef([[
  typedef struct { float x, y, z; } Vec3f;
  typedef struct { bool hit; float t, x, y, z, nx, ny, nz; unsigned entity; } RayHit;
  void xfSet( unsigned e, float x, float y, float z );
  Vec3f xfGet( unsigned e );
  void xfSetRot( unsigned e, float yawDeg, float pitchDeg, float rollDeg );
  float xfGetYaw( unsigned e );
  void xfSetQuat( unsigned e, float x, float y, float z, float w );
  void xfSetScale( unsigned e, float x, float y, float z );
  void xfSetModelYaw( unsigned e, float deg );
  void xfSetVisible( unsigned e, bool v );
  void xfRemove( unsigned e );
  void bodySet( unsigned e, int kind, float r, float h, float d, unsigned collType, bool solid );
  void bodyTune( unsigned e, float gravity, float jumpSpeed, float accelGround, float accelAir, float maxSlopeDeg, float stepHeight );
  void bodyWish( unsigned e, float vx, float vz );
  void bodyMove( unsigned e, float right, float fwd );
  void bodyTurnRate( unsigned e, float rate );
  void bodyJump( unsigned e );
  void bodySetVel( unsigned e, float x, float y, float z );
  Vec3f bodyGetVel( unsigned e );
  bool bodyGrounded( unsigned e );
  bool bodyJustLanded( unsigned e );
  bool levelAddCollision( const char* bmeshPath );
  void levelBuild( float cell );
  void levelClear();
  unsigned levelTriangles();
  RayHit raycast3D( float ox, float oy, float oz, float dx, float dy, float dz, float maxDist, unsigned ignore );
  void motionLinear3D( unsigned e, float speed, bool pingPong );
  void motionWaypoint3D( unsigned e, float x, float y, float z );
  void motionOrbit3D( unsigned e, unsigned center, float cx, float cy, float cz, float r, float degPerSec, float phaseDeg, float bobAmp, float bobHz );
  void motionFollow3D( unsigned e, unsigned tgt, float ox, float oy, float oz, float rate );
  void motionSpin3D( unsigned e, float degPerSec );
  void motionView3D( unsigned e, unsigned tgt, float right, float up, float fwd );
  void motionKick3D( unsigned e, float back, float pitchDeg, float rate );
  void cam3dRig( unsigned camera, int mode, unsigned target );
  void cam3dParams( float distance, float height, float eyeHeight, float rate, bool collide, float sideDistance );
  void cam3dPitchLimits( float minDeg, float maxDeg );
  void cam3dLook( float dYawDeg, float dPitchDeg );
  void cam3dTurn( float yawDegPerSec, float pitchDegPerSec );
  void cam3dSetAngles( float yawDeg, float pitchDeg );
  float cam3dYaw();
  float cam3dPitch();
  void cam3dFreeVel( float right, float up, float back );
  Vec3f cam3dEye();
  float cam3dViewYaw();
  float cam3dViewPitch();
  void view3dFov( float deg, float nearZ, float farZ );
  void view3dSky( float r, float g, float b );
  void view3dFog( float r, float g, float b, float nearZ, float farZ );
  void view3dLight( float dx, float dy, float dz, float r, float g, float b, float ambient, bool shadows );
  void view3dBillboardPixel( float meters );
  void view3dCrosshair( bool on );
  void view3dShadowQuality( int q );
  void playSoundFXAt( int key, int vel, float x, float y, float z, float ref, float maxDist );
]])
local C = ffi.C
local W = {}
local rad, deg = math.rad, math.deg

-- The level: every object of an exported level file (genome/level/<name>.lua, tools/blender). role "static" is drawn
-- and collides, "decor" is only drawn, "collider" only collides (invisible walls), "character" is skipped (genome
-- models, drawn where entities are placed). Returns the level table (its entities and spawns are Scene.lua's).
-- level: a bare level name ("courtyard") or an old file path.
function W.level( scenePath, cell )
  if not scenePath:find( "/", 1, true ) then scenePath = require( "Ids" ).asset( "level", scenePath ) end
  -- mesh names in the list are relative to the game's genome/ (e.g. "model/ground")
  local dir = scenePath:match( "^(.*/genome/)" ) or scenePath:match( "^(.*/)" )
  local scene = Asset.dofile( scenePath )
  for _, obj in ipairs( scene.objects or {} ) do
    local mesh = dir..obj.mesh..".bmesh"
    if obj.role == "static" or obj.role == "decor" then
      if not ffi.C.addStaticModel( mesh ) then error( "3D: can't load "..mesh.." (build the 'assets' target)" ) end
    end
    if obj.role == "static" or obj.role == "collider" then
      if not C.levelAddCollision( mesh ) then error( "3D: no collision mesh "..mesh ) end
    end
  end
  C.levelBuild( cell or 2 )
  return scene
end

-- Bodies -------------------------------------------------------------------------------------------------------------
-- opts: kind = "character" | "kinematic" | "box" | nil (transform only), x, y, z, yaw, radius, height,
--       half = { x, y, z } (box), collType (nonzero: onCollision/onUncollision reach the entity), solid,
--       model (mesh name under <game>/genome/model), modelYaw, scale, image (a billboard sprite: a .bimg path),
--       gravity, jumpSpeed, accel, airAccel, maxSlope, step
W.Body = {}
W.Body.__index = W.Body
local KIND = { character = 1, kinematic = 2, box = 3 }
-- Scene changes (scripts/Scene.lua): bodies made while a scene is loading belong to it and are despawned when it's
-- left; persist = true bodies (the player, a companion) are carried to the next scene. player = true marks the one
-- placed on the next scene's spawn point (companions keep their offset from it).
W.live = {}        -- this scene's bodies
W.persistent = {}  -- id -> body
W.persistentByName = {}  -- genome name -> its persistent body (Genome.spawn3d): not spawned again by the next scene
W.player = false
local function scoped( persist, fn )
  local S = package.loaded["Scene"]
  if S and type( S ) == "table" and S.withScope then return S.withScope( not persist, fn ) end
  return fn()
end
function W.Body.new( opts )
  return scoped( opts.persist, function() return W.Body.make( opts ) end )
end
function W.Body.make( opts )
  local self = setmetatable( { id = opts.id or ffi.C.newEntity() }, W.Body )
  if opts.persist then
    W.persistent[self.id] = self
    if opts.player then W.player = self end
  else
    table.insert( W.live, self )
  end
  local id = self.id
  C.xfSet( id, opts.x or 0, opts.y or 0, opts.z or 0 )
  C.xfSetRot( id, opts.yaw or 0, opts.pitch or 0, 0 )
  if opts.kind then
    local k = KIND[opts.kind] or error( "3D: unknown body kind "..tostring( opts.kind ) )
    if k == 3 then
      local h = opts.half or { 0.5, 0.5, 0.5 }
      C.bodySet( id, k, h[1], h[2], h[3], opts.collType or 0, true )
    else
      C.bodySet( id, k, opts.radius or 0.35, opts.height or 1.6, 0, opts.collType or 0, opts.solid or false )
    end
  end
  if opts.gravity or opts.jumpSpeed or opts.accel or opts.maxSlope or opts.step then
    C.bodyTune( id, opts.gravity or 25, opts.jumpSpeed or 9, opts.accel or 60, opts.airAccel or 18, opts.maxSlope or 50, opts.step or 0.35 )
  end
  if opts.model then
    if not ffi.C.setModel( id, require( "Ids" ).asset( "model", opts.model ) ) then
      error( "3D: can't load model "..opts.model )
    end
  end
  if opts.image then
    if not ffi.C.newPixelImage( id, opts.image ) then error( "3D: can't load image "..opts.image ) end
  end
  if opts.modelYaw then C.xfSetModelYaw( id, opts.modelYaw ) end
  if opts.turnRate then C.bodyTurnRate( id, opts.turnRate ) end
  if opts.scale then
    local s = type( opts.scale ) == "number" and { opts.scale, opts.scale, opts.scale } or opts.scale
    C.xfSetScale( id, s[1], s[2], s[3] )
  end
  if opts.collType and opts.collType ~= 0 then register( id, self ) end  -- so overlaps reach onCollision
  return self
end

-- A door in a 3D scene: an invisible, non-solid trigger capsule (radius, height) that changes scenes when the
-- player (collType 1) walks into it, like the door gene of 2D scenes.
--   W.door{ x, y, z, radius = 1, height = 2.5, to = "bonus", spawn = "entry", on = "touch" | "press", transition }
function W.door( opts )
  local d = W.Body.new{ kind = "kinematic", x = opts.x, y = opts.y, z = opts.z, radius = opts.radius or 1,
                        height = opts.height or 2.5, collType = 6 }
  d.door = { to = opts.to, spawn = opts.spawn, on = opts.on, transition = opts.transition, duration = opts.duration,
             music = opts.music }
  require( "Scene" ).armDoor( d )
  return d
end
function W.Body:pos() local p = C.xfGet( self.id ) return p.x, p.y, p.z end
function W.Body:setPos( x, y, z ) C.xfSet( self.id, x, y, z ) end
function W.Body:setYaw( d ) C.xfSetRot( self.id, d, 0, 0 ) end
function W.Body:yaw() return C.xfGetYaw( self.id ) end
function W.Body:wish( vx, vz ) C.bodyWish( self.id, vx, vz ) end
-- Camera-relative movement, set when it changes (a key press or release), not every frame: (right, forward) m/s
-- relative to wherever the camera faces, re-aimed by the physics step as the camera turns. With the body's
-- turnRate (body gene) it also turns to face the way it moves. wish() (world space) switches this off.
function W.Body:move( right, fwd ) C.bodyMove( self.id, right, fwd ) end
function W.Body:jump() C.bodyJump( self.id ) end
function W.Body:vel() local v = C.bodyGetVel( self.id ) return v.x, v.y, v.z end
function W.Body:setVel( x, y, z ) C.bodySetVel( self.id, x, y, z ) end
function W.Body:grounded() return C.bodyGrounded( self.id ) end
function W.Body:justLanded() return C.bodyJustLanded( self.id ) end
function W.Body:show( v ) C.xfSetVisible( self.id, v ) end
-- Gone at once (no collisions, not drawn; queued events to/from it voided), its id freed at the end of the frame.
W.dying = {}
function W.Body:remove()
  if self.removed then return end
  self.removed = true
  C.xfRemove( self.id )
  table.insert( W.dying, self.id )
  unregister( self.id )
  W.persistent[self.id] = nil
  if self.genomeName and W.persistentByName[self.genomeName] == self then W.persistentByName[self.genomeName] = nil end
  for i, b in ipairs( W.live ) do if b == self then table.remove( W.live, i ) break end end
end
-- One of the body's genome sounds (its sfx gene; scripts/Sound.lua), played where the body is.
function W.Body:sound( name, vel )
  local x, y, z = self:pos()
  require( "Sound" ).playFor( self, name, vel, x, y, z )
end
-- Timers (onTimer( self, timerId, type )), as on 2D genome instances.
function W.Body:after( ms, ttype ) return C.createTimer( ms, self.id, ttype or 0, false, C.TO_BRIDGE ) end
function W.Body:every( ms, ttype ) return C.createTimer( ms, self.id, ttype or 0, true, C.TO_BRIDGE ) end
function W.Body:stopTimer( id ) C.stopTimer( id ) end
-- Kinematics-driven motions (the same system as the 2D motions, on the transform).
function W.Body:path( points, speed, pingPong )
  for _, p in ipairs( points ) do C.motionWaypoint3D( self.id, p[1], p[2], p[3] ) end
  C.motionLinear3D( self.id, speed or 2, pingPong or false )
end
function W.Body:orbit( opts )
  local c = opts.center or { 0, 0, 0 }
  C.motionOrbit3D( self.id, opts.around or 0, c[1], c[2], c[3], opts.radius or 2, opts.speed or 45, opts.phase or 0, opts.bob or 0, opts.bobHz or 1 )
end
function W.Body:follow( target, opts )
  opts = opts or {}
  local o = opts.offset or { 0, 0, 0 }
  if opts.view then  -- ride in the target's view: offset = { right, up, forward }, turned with its yaw and pitch
    C.motionView3D( self.id, target, o[1], o[2], o[3] )
    return
  end
  C.motionFollow3D( self.id, target, o[1], o[2], o[3], opts.mode == "hard" and 0 or ( opts.rate or 6 ) )
end
-- Recoil for a view rider (follow{ view = true }): back meters and pitchDeg up, easing out at rate per second.
function W.Body:kick( back, pitchDeg, rate ) C.motionKick3D( self.id, back or 0.06, pitchDeg or 6, rate or 8 ) end
function W.Body:spin( degPerSec ) C.motionSpin3D( self.id, degPerSec ) end

-- Raycast (hitscan): origin and direction tables { x, y, z }; returns nil or { x, y, z, nx, ny, nz, t, entity }
-- (entity 0 = the level).
function W.raycast( o, d, maxDist, ignore )
  local h = C.raycast3D( o[1], o[2], o[3], d[1], d[2], d[3], maxDist or 1000, ignore or 0 )
  if not h.hit then return nil end
  return { x = h.x, y = h.y, z = h.z, nx = h.nx, ny = h.ny, nz = h.nz, t = h.t, entity = h.entity }
end

-- Camera rigs: the scene camera is a kinematic body Kinematics drives toward its target each frame.
--   W.Camera.side( id, { distance = 9, height = 1.4 } )
--   W.Camera.orbit( id, { distance = 6, height = 1.4, rate = 10, collide = true, yaw = 0, pitch = -20 } )
--   W.Camera.firstPerson( id, { eye = 1.6 } )
--   W.Camera.free( { x, y, z } )                         -- debug fly-cam; W.Camera.fly( right, up, back )
--   W.Camera.look( dYawDeg, dPitchDeg )                  -- feed mouse / stick deltas
--   W.Camera.fov( 70 )
W.Camera = {}
function W.Camera.id() return ffi.C.cameraEntity() end
local function rig( mode, target, opts )
  opts = opts or {}
  W.Camera.last = { mode = mode, target = target or 0, opts = opts }  -- re-applied after a scene change
  C.cam3dParams( opts.distance or 6, opts.height or 1.4, opts.eye or 1.6, opts.mode == "hard" and 0 or ( opts.rate or 10 ),
                 opts.collide ~= false, opts.distance or 9 )
  if opts.minPitch or opts.maxPitch then C.cam3dPitchLimits( opts.minPitch or -83, opts.maxPitch or 70 ) end
  if opts.yaw or opts.pitch then C.cam3dSetAngles( opts.yaw or C.cam3dYaw(), opts.pitch or C.cam3dPitch() ) end
  C.cam3dRig( ffi.C.cameraEntity(), mode, target or 0 )
  if opts.fov then C.view3dFov( opts.fov, 0, 0 ) end
end
function W.Camera.side( target, opts ) rig( 1, target, opts ) end
function W.Camera.orbit( target, opts ) rig( 2, target, opts ) end
function W.Camera.firstPerson( target, opts ) rig( 3, target, opts ) end
function W.Camera.free( at, opts )
  if at then C.xfSet( ffi.C.cameraEntity(), at[1], at[2], at[3] ) end
  rig( 4, 0, opts )
end
function W.Camera.off() W.Camera.last = false C.cam3dRig( ffi.C.cameraEntity(), 0, 0 ) end
-- After a scene change: the same rig on the same (persistent) target, keeping the current look angles.
function W.Camera.reapply()
  local l = W.Camera.last
  if not l then return end
  local keep = {}
  for k, v in pairs( l.opts ) do if k ~= "yaw" and k ~= "pitch" then keep[k] = v end end
  rig( l.mode, l.target, keep )
end
function W.Camera.look( dy, dp ) C.cam3dLook( dy, dp ) end
-- Keeps turning (degrees per second) until told otherwise: held camera keys set it on press, clear it on release.
function W.Camera.turn( yawRate, pitchRate ) C.cam3dTurn( yawRate or 0, pitchRate or 0 ) end
function W.Camera.angles() return C.cam3dYaw(), C.cam3dPitch() end
function W.Camera.setAngles( y, p ) C.cam3dSetAngles( y, p ) end
function W.Camera.fly( r, u, b ) C.cam3dFreeVel( r, u, b ) end
function W.Camera.fov( d ) C.view3dFov( d, 0, 0 ) end
function W.Camera.eye() local e = C.cam3dEye() return e.x, e.y, e.z end
-- The direction the camera looks (unit { x, y, z }).
function W.Camera.forward()
  local y, p = rad( C.cam3dViewYaw() ), rad( C.cam3dViewPitch() )
  return { -math.sin( y ) * math.cos( p ), math.sin( p ), -math.cos( y ) * math.cos( p ) }
end
-- Camera-relative movement: stick/WASD (right, forward) -> world (vx, vz) on the ground plane.
function W.Camera.relative( right, fwd )
  local y = rad( C.cam3dYaw() )
  local fx, fz = -math.sin( y ), -math.cos( y )
  local rx, rz = math.cos( y ), -math.sin( y )
  return rx * right + fx * fwd, rz * right + fz * fwd
end

-- View: sky, fog, light (direction the light travels), shadows.
W.View = {}
function W.View.sky( r, g, b ) C.view3dSky( r, g, b ) end
function W.View.fog( r, g, b, near, far ) C.view3dFog( r, g, b, near, far ) end
function W.View.light( dir, color, ambient, shadows )
  color = color or { 1, 0.97, 0.9 }
  C.view3dLight( dir[1], dir[2], dir[3], color[1], color[2], color[3], ambient or 0.38, shadows ~= false )
end
function W.View.billboardPixel( m ) C.view3dBillboardPixel( m ) end
function W.View.crosshair( on ) C.view3dCrosshair( on ) end
-- Shadow quality: "off" | "low" (512 map, 1 tap) | "med" (1024, 4-tap PCF) | "high" (2048, 9-tap) | "auto" (default:
-- low on Raspberry Pi / Mali / llvmpipe, high otherwise). The BICYCLE_SHADOWS environment variable overrides it.
local SHADOW_Q = { auto = -1, off = 0, low = 1, med = 2, high = 3 }
function W.View.shadows( q ) C.view3dShadowQuality( SHADOW_Q[q or "auto"] or -1 ) end

-- Positional sound: a SoundFont SFX key at a point, panned/attenuated for the camera.
function W.sound( key, vel, x, y, z, ref, maxDist ) C.playSoundFXAt( key, vel or 100, x, y, z, ref or 3, maxDist or 60 ) end

-- Engine events from 3D body state (so games don't poll it every frame): a character's onLand( self, speed ) when
-- it touches down, and onFallOut( self ) when it drops below its killY (a genome field, default -15).
local function bodyEvents()
  local function check( b )
    if b.removed then return end
    if b.onLand and C.bodyJustLanded( b.id ) then b:onLand() end
    if b.onFallOut then
      local _, y = b:pos()
      if y < ( b.killY or -15 ) then b:onFallOut() end
    end
  end
  for _, b in ipairs( W.live ) do check( b ) end
  for _, b in pairs( W.persistent ) do check( b ) end
end
table.insert( engineFrameHooks, bodyEvents )

return W
