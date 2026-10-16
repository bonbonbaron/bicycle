# True 3D (third-person platformers, first-person shooters)

3D mode (`-DBICYCLE_RENDER=3D`) has two paths. Without a camera rig it is the 2.5D side view of a 2D game
(unchanged). Once Lua picks a camera rig, the world is genuinely 3D: entities get 3D transforms and bodies, the
level is a collision mesh, and the renderer draws from the camera entity's transform. ASCII and 2D builds compile
the same code and are unaffected (2D games never create a transform).

Units: meters, seconds, degrees in Lua (radians in C++), Y up, yaw 0 looks down -Z, an entity's position is its
feet. Demos: `./o-3d plat3d` (third person) and `./o-3d fps3d` (first person).

| Piece | Where |
| --- | --- |
| math (vec3, quat, mat4, frustum, ray/triangle, closest points, stereo pan) | `include/m/Math3D.h` |
| transforms, bodies, level collider, character physics, raycasts, overlaps | `include/c/Physics3D.h`, `src/c/Physics3D.cpp` |
| 3D motions + camera rigs (side, orbit, first person, free) | `Kinematics::linear3D/orbit3D/follow3D/spin3D/runCamera3D` |
| FFI | `src/c/World3DApi.cpp`, `src/c/InputState.cpp` |
| Lua | `scripts/World3D.lua`, `scripts/Input3D.lua`, `onFrame(dt)` in `scripts/bicycle.lua` |
| rendering (shadow map, fog, culling, billboards) | `src/v/3d/Renderer3D.cpp` (`drawWorld3D`) |
| levels from Blender | `scripts/blender/make_level.py` + `export.py` |
| tests | `tests/math3d_test.cpp` (`ctest -R math3d`) |

## Physics

- Characters are capsules (`radius`, `height`). Each step: steer the horizontal velocity toward `wish` (ground or
  air acceleration), gravity, jump (with 0.12 s coyote time), then move in sub-steps of 0.4 r, resolving the
  deepest contact at a time against level triangles, kinematic boxes and solid bodies. Walkable contacts
  (`maxSlope`, default 50 degrees) push straight up (no sliding on slopes) and set `grounded`; walls slide.
  Blocked while grounded: try a step up of `step` (0.35 m) and settle back down. Walking off a ledge or down
  stairs snaps to the ground within a step.
- The level is a uniform XZ grid (2 m cells, CSR arrays) of triangles from the same meshes the renderer draws
  (`.bmesh` from the pack, or a dev build's loose `.obj`). Raycasts walk the grid with a 2D DDA and stop at the
  first cell boundary past the best hit; entities are tested as three spheres per capsule or as boxes.
- `box` bodies are kinematic axis-aligned boxes (moving platforms): characters standing on one move with it.
- Overlaps between bodies with a `collType` call Lua's `onCollision(other, type)` / `onUncollision`.
- No allocation while running: fixed arrays over `NUM_SUPPORTED_ENTITIES`, the grid is built once at load.

## Lua

```lua
local W = require("World3D")
local Input = require("Input3D")
W.level( "./mygame/genome/models/scene3d.lua" )      -- static: draw + collide; decor: draw; collider: collide
W.View.light( { -0.45, -1, -0.35 }, { 1, 0.96, 0.88 }, 0.4, true )   -- direction, color, ambient, shadows
W.View.fog( 0.62, 0.76, 0.95, 25, 70 )
local hero = W.Body.new{ kind = "character", x = 0, y = 1, z = 8, model = "pilgrim3d", modelYaw = 90, collType = 1 }
W.Camera.orbit( hero.id, { distance = 7, height = 1.5, rate = 12, pitch = -18, fov = 65 } )   -- or .firstPerson/.side/.free
Input.bind( "jump", { "space", "pad:a" } )
Input.bindAxis( "moveX", { neg = { "a" }, pos = { "d" }, pad = "leftx" } )
Input.bindAxis( "moveY", { neg = { "s" }, pos = { "w" }, pad = "lefty", padScale = -1 } )
Input.captureMouse( true )      -- SDL relative mouse mode
Input.enableGamepad()
function onFrame( dt )
  local dx, dy = Input.look( dt )                       -- mouse + right stick
  W.Camera.look( -dx * 0.25, -dy * 0.2 )                -- the rig places the camera; Lua only steers it
  hero:wish( W.Camera.relative( Input.axis( "moveX" ) * 7, Input.axis( "moveY" ) * 7 ) )
  if Input.pressed( "jump" ) and hero:grounded() then hero:jump() end
end
local hit = W.raycast( { W.Camera.eye() }, W.Camera.forward(), 100, hero.id )   -- hitscan: hit.entity, hit.x ...
W.sound( 72, 110, x, y, z )                            -- positional SFX: pan + attenuation from the camera
coin:spin( 180 )  platform:path( { { 2, 0.75, -14 }, { 2, 4, -14 } }, 1.2, true )   -- Kinematics 3D motions
```

## Headless capture

`BICYCLE_SCRIPTED_INPUT` presses also count as held keys on their frames; `BICYCLE_SCRIPTED_MOUSE="12:0@30-60;L@80-81"`
adds mouse motion (dx:dy per frame) or held buttons (L/R/M) over frame ranges. `BICYCLE_FRAME_STATS=1` prints
per-frame physics / Lua / render milliseconds and meshes drawn/culled. `BICYCLE_NO_SHADOWS=1` disables the shadow map.

## Limits

- Only one SFX channel: overlapping positional sounds share the latest pan.
- Static level collision only for meshes; moving collision is boxes. No rotated boxes, no dynamic rigid bodies.
- Rigged models: walk/idle are picked from motion; no blending, no jump/fall actions in the demo character.
- One shadow cascade around the camera's focus (22 m each side); no skybox texture (sky color + fog).
