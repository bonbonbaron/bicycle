# bicycle: true 3D (branch `true-3d`, worktree /workspace/bicycle-3d)

## Where we start (grounded in the code)
- `src/v/3d/Renderer3D.cpp`: 2.5D. `drawWorld()` maps 2D boxes to X/Y (`x = cells/16`, `y = rows/8`, z fixed), camera
  hard-coded `eye {camCx,3.2,8}`; lambert-ish fixed light, no fog/shadows/culling. Billboards are XY quads.
- `Kinematics` (include/c/Kinematics.h): 2D only (`motion::Vec2`, `Box` fixed point). Camera = entity driven by
  `FOLLOW_LERP` (`Camera.track` in scripts/bicycle.lua).
- Collision: `CollisionDetector` 2D boxes -> `Trigger::sendCollision` -> Lua `onCollision`.
- Input: SDL key-down events only (no held state, mouse or gamepad); `BICYCLE_SCRIPTED_INPUT` injects presses.
- Audio: fluidsynth, one SFX channel (15), no pan.
- Lua per-frame hook: none (only event callbacks in `getUpdates`).

## Design
3D is an *opt-in layer* next to the 2D one: an entity becomes 3D when Lua gives it a `Transform3D`/`Body3D`.
2D games never touch it, so ASCII/2D keep `Box` + `Vec2` unchanged. All 3D state is fixed-size arrays over
`NUM_SUPPORTED_ENTITIES` or `mem::Vector`s in GENERAL/GRAPHIC reserved at load: no per-frame heap traffic.
- `include/m/Math3D.h` (header-only, unit tested): Vec3, Quat, Mat4, yaw/pitch, ray/tri, sphere/tri closest point,
  frustum planes, stereo pan/attenuation.
- `include/c/Physics3D.h` + `src/c/Physics3D.cpp`: transforms, bodies (velocity, gravity, jump, capsule),
  static level collider (triangles from the same .bmesh/.obj the renderer draws, uniform XZ grid), capsule-vs-mesh
  slide with ground/slope/step handling, raycast vs level + entity spheres, entity-vs-entity sphere/capsule
  overlap -> the existing Collision bridge (Lua `onCollision`). Stepped from `Kinematics::run()`.
- Kinematics 3D motions: linear3D waypoints, orbit3D, follow3D; camera **rigs** (side / orbit / first-person / free)
  are Kinematics-driven motions of the camera entity targeting an entity; Lua only sets mode and feeds yaw/pitch.
- Renderer3D: when a 3D camera rig is active, draws from the camera entity's transform with configurable FOV;
  level meshes, entity models at their transforms, camera-facing billboards; directional light + shadow map,
  ambient, fog, sky clear color, frustum culling. Without a rig the old 2.5D path is unchanged (test game).
- Input: held keys, mouse delta (SDL relative mouse mode), mouse buttons, SDL3 gamepad axes/buttons into an
  `InputState` read by Lua through FFI; `scripts/Input3D.lua` maps actions. Scripted mouse for headless runs.
- Audio: `playSoundFXAt(key, vel, x,y,z)`: pan (CC10) + gain from the camera (listener) transform.
- Lua: `onFrame(dt)` hook; `scripts/World3D.lua` wrapper (`Body3D`, `Camera3D`, `Physics3D.raycast`).

## Milestones
- **M1** 3D world core: Math3D, Transform3D/Body3D, gravity/velocity/jump, 3D motions; tests (`ctest -R math3d`).
- **M2** Camera rigs from Lua via Kinematics (side, orbit w/ lerp + push-in, FPS, free), FOV, perspective.
- **M3** Level collider (grid), capsule slide, ground/slope/steps, raycast/hitscan, entity overlap; Lua API; tests.
- **M4** Renderer: multi-mesh levels via Blender->.bmesh, lambert+ambient, shadow map, fog, sky, frustum culling,
  camera-facing billboards.
- **M5** Input: held keys, mouse look, camera-relative WASD, gamepad, Lua action map, scripted mouse.
- **M6** Positional SFX (pan + attenuation).
- **M7** Demos `plat3d/` (3rd person) and `fps3d/` (1st person), levels built by `scripts/blender/make_level.py`
  headless (Blender 4.5.9 at /home/box/blender/blender-4.5.9-linux-x64), dev loose + release embedded;
  Xvfb screenshots to /workspace/out.
- Verification: ASCII/2D/3D dev+release builds, ctest (codecs, motion, math3d/physics, release-zero-io),
  llvm-mingw Windows cross builds.

Note: game folders live at the repo root (like `test/`): the engine resolves `./<game>/main.lua` and the packer
packs one game folder, so the demos are `plat3d/` and `fps3d/` rather than `games/...`.
