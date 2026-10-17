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
| levels and placed genomes from Blender | `tools/blender/bicycle_blender.py` (add-on; [blender-entities.md](blender-entities.md)), `scripts/blender/make_level.py` + `export.py` |
| genomes as 3D bodies | `Genome.spawn3d` in `scripts/Genome.lua` (README, "Self-contained genomes") |
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
W.level( "scene3d" )                                 -- genome/level/scene3d.lua. static: draw + collide; decor: draw; collider: collide
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

## Scene changes in true 3D

`Scene.go` (README, "Scene changes") works for true-3D games. A scene file sets `world3d` instead of `bg`/`fg`, and
is only the scene's look; what's in it is placed in Blender:

```lua
-- plat3d/genome/scene/bonus.lua
return { name = "Bonus room",
  world3d = { level = "bonus",                      -- genome/level/bonus.lua, exported by tools/blender
              sky = { r, g, b }, fog = { r, g, b, near, far }, light = { dir, color, ambient, shadows } } }
```

The level file has the level's meshes, its `entities` (genomes placed in Blender, each `{ genome = "coin", key,
pos = { x, y, z }, yaw, <per-instance fields> }`) and its `spawns = { entry = { x, y, z, yaw } }`. On load, every
entity is spawned from its genome (`Genome.spawn3d`): `model`, `body = { kind, radius, height, half, pitch, ... }`,
`coll`, `motion` (`spin`/`linear`/`orbital`/`follow`, run by Kinematics), `door` (a trigger to another scene),
`sfx`, `timer` and the event callbacks. A scene can still add `world3d.spawns`, `world3d.entities` (hand-placed)
or an old `build = function( scene ) ... end`.

- Start with `Scene.start( "courtyard" )` (it pushes the scene window: create bodies after it, entity 0 is reserved).
- `persist = true, player = true` (a genome's fields or `W.Body.new` options): carried to the next scene, the player
  onto the spawn point (velocity zeroed, yaw from the spawn), other persistent bodies at the same offset;
  `onSceneEnter( name, spawn )`. A persistent genome that's already alive isn't placed again when its level comes
  back up, and `Scene.removeForever( body )` (a collected coin) keeps it out of that scene for good.
- Everything else made while the scene loaded is despawned on the way out (transform, body, overlaps with an
  uncollision to the other side, motions, model), the level collision is freed (`levelClear`), and the scene's
  meshes, textures and collision grid (GRAPHIC, i.e. the SCENE pool during the load) go with the SCENE reset.
- The camera rig (`W.Camera.orbit/firstPerson/side/free`) is re-applied on the same target with the same options,
  keeping the look angles, and the camera starts where it was relative to the player.
- A `door = { to, spawn, radius, height, on, transition, duration, music }` gene makes a genome a non-solid trigger
  capsule (collType 6) for the player (collType 1); the ID generator and the packer check its target scene and
  spawn exist. (`W.door{ x, y, z, ... }` does the same from a script.)
- Demo: `./o-3d plat3d`: touch the flag on the tower to enter the bonus room; its flag leads back. Coins stay
  collected per scene (`Scene.state`). `PLAT3D_SCENE=bonus ./o-3d plat3d` starts there. The analog controls
  (camera-relative running, mouse look) are the game's `onFrame` in `main.lua`; the genomes only have events.

## Headless capture

`BICYCLE_SCRIPTED_INPUT` presses also count as held keys on their frames; `BICYCLE_SCRIPTED_MOUSE="12:0@30-60;L@80-81"`
adds mouse motion (dx:dy per frame) or held buttons (L/R/M) over frame ranges. `BICYCLE_FRAME_STATS=1` prints
per-frame physics / Lua / render milliseconds and meshes drawn/culled. `BICYCLE_NO_SHADOWS=1` disables the shadow map.

## Limits

- Only one SFX channel: overlapping positional sounds share the latest pan.
- Static level collision only for meshes; moving collision is boxes. No rotated boxes, no dynamic rigid bodies.
- Rigged models: walk/idle are picked from motion; no blending, no jump/fall actions in the demo character.
- One shadow cascade around the camera's focus (22 m each side); no skybox texture (sky color + fog).
