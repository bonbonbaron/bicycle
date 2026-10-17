# Raspberry Pi

| Model | CPU / OS | GPU (Mesa) | ASCII | 2D | 3D |
|---|---|---|---|---|---|
| Pi 4 / 400 | Cortex-A72, 64-bit (aarch64) or 32-bit | VideoCore VI, v3d: GL 3.1 / GLES 3.1 | yes | yes | GLES3 tier (GLSL 300 es, VAOs): shadows on by default, at `low` quality |
| Pi Zero 2 W | Cortex-A53, 64-bit or 32-bit | VideoCore IV, vc4: GL 2.1 / GLES 2.0 | yes | yes | GLES2 tier, no shadows |
| Pi Zero / Zero W | ARM1176 (ARMv6), 32-bit armhf | VideoCore IV, vc4: GLES 2.0 | yes | yes (software framebuffer; slow-ish) | GLES2 tier, no shadows; expect a low frame rate |

## 3D renderers
The 3D mode tries three context tiers, best first, and logs the one it got at startup
(`bicycle 3D: OpenGL ES 3.0 tier (GL ... on ...)`):
1. **core**: OpenGL 3.3 core (GLSL 330, VAOs, shadow map). Desktop GPUs.
2. **gles3**: OpenGL ES 3.0+ (the same shaders as `#version 300 es` with precision qualifiers; VAOs; a
   16-bit depth shadow map sampled with `sampler2DShadow`; BGRA textures via `GL_EXT_texture_format_BGRA8888` where
   the driver has it, else swizzled to RGBA on the CPU). The Pi 4's v3d (GLES 3.1, desktop GL only 3.1) lands here,
   so **the Pi 4 gets shadows by default** (at `low` quality, see below).
3. **gles2**: OpenGL ES 2.0 (GLSL ES 1.00, attributes rebound per draw, no shadow map). The Pi Zero / Zero 2's vc4.

If a tier's context can't be created, the window is destroyed and recreated before the next tier is tried (X11/EGL and
Wayland can tie a window to the first config attempted). `BICYCLE_GL=core|gles3|gles2` picks the first tier to try
(lower tiers stay as fallbacks); `BICYCLE_GLES2=1` or `-DBICYCLE_GLES2=ON` is the same as `BICYCLE_GL=gles2`.
No flags are needed on a Pi: `./o-3d test`. On a Zero, `BICYCLE_GL=gles2` skips the two failing attempts.

## Shadows and frame timing (3D)
`BICYCLE_SHADOWS=off|low|med|high|auto` (or `W.View.shadows( "low" )` from Lua; the environment wins):

| Quality | Map | Depth compares per pixel | Covers |
|---|---|---|---|
| off | none | none: no shadow pass, the same cost as gles2 | |
| low | 512 | 1 (bilinear PCF where the GPU filters depth) | the camera frustum out to 16 m |
| med | 1024 | 4 (PCF) | the camera frustum out to 24 m |
| high | 2048 | 9 (3x3 PCF) | 44 m square around the player |

`auto` (the default) is `low` when GL_RENDERER names v3d, vc4, Mali, Adreno, PowerVR or llvmpipe, `high` otherwise;
startup logs the choice (`bicycle 3D: shadows low (auto; ...)`). The shadow map is only redrawn when the light view or
a caster changed: level geometry goes into its own map, redrawn only when the light view moves (the camera moves by
more than a shadow texel or turns), and is copied in before the moving casters are drawn. On ES the maps are
`GL_DEPTH_COMPONENT16`. Shadows cost on two sides: the extra depth pass, and in the main pass the per-pixel lookups,
which is where most of the time goes on a tiled/software GPU. Try `low`, `med`, and `off` on the Pi with:

`BICYCLE_FPS=1 ./o-3d plat3d` prints once a second, e.g.
`fps 30.2 | ms/frame: shadow 1.40 main 3.95 billboards 0.01 overlay 1.61 swap 0.70 | shadow maps drawn 31/31 (statics 0) | draws 45 | shadows low | OpenGL ES 3.0`
(it calls glFinish after each pass so the per-pass times include the GPU, which costs a little itself; `shadow` also
includes picking and posing what's drawn, `overlay` the UI layer upload). The game loop runs at 30 fps, so on a fast
machine fps stays at 30 and the ms show the headroom; on the Pi watch for fps under 30.

2D draws into a software framebuffer and SDL presents it with whatever renderer works (GLES2 on vc4/v3d, or
`SDL_RENDER_DRIVER=software`).

## Building on the Pi (native, recommended)
```sh
sudo apt install git cmake ninja-build build-essential pkg-config \
  libasound2-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev \
  libwayland-dev libxkbcommon-dev libdrm-dev libgbm-dev libegl-dev libgles-dev libncurses-dev
git clone https://github.com/bonbonbaron/bicycle && cd bicycle && git checkout pi-zero2   # later: git pull
cmake -B build-2d -G Ninja -DBICYCLE_RENDER=2D -DCMAKE_BUILD_TYPE=Release && cmake --build build-2d
./o-2d test
cmake -B build-3d -G Ninja -DBICYCLE_RENDER=3D -DCMAKE_BUILD_TYPE=Release && cmake --build build-3d
./o-3d test
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build && ./o test   # ASCII, also over ssh
```
The first configure builds LuaJIT, fluidsynth and SDL3 from source: allow ~10 minutes on a Pi 4, an hour or more
on a Zero (use `-j1` / a swap file on 512 MB boards: `cmake --build build-2d -j1`). The dev packages above give SDL
its X11, Wayland and KMS/DRM (console, no desktop) video backends and ALSA sound.

## Cross-building (x86_64 Linux, zig 0.14)
Wrapper scripts in `$ZIG_WRAP` (CMake wants a single compiler path):
`a64-cc`: `exec zig cc -target aarch64-linux-gnu.2.36 -mcpu=cortex_a53 "$@"` (runs on A53 and A72),
`arm6-cc`: `exec zig cc -target arm-linux-gnueabihf.2.36 -mcpu=arm1176jzf_s -mfloat-abi=hard "$@"`, the same with
`c++`, and `*-ar`/`*-ranlib` running `zig ar`/`zig ranlib`.
```sh
export ZIG_WRAP=$HOME/zigwrap
cmake -B build-pi-aarch64-2D -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-zig.cmake -DBICYCLE_RENDER=2D \
  -DCMAKE_BUILD_TYPE=Release -DBICYCLE_FETCH_NCURSES=ON -DBICYCLE_OUTPUT_DIR=$PWD/build-pi-aarch64-2D/bin
cmake --build build-pi-aarch64-2D
# Pi Zero / Zero W: cmake/armv6-zig.cmake (needs gcc-multilib: LuaJIT's buildvm is built with "gcc -m32")
```
`BICYCLE_OUTPUT_DIR` keeps the Pi executables out of the repo root. Binaries target glibc 2.36 (bookworm) and need
only libc/libm; SDL loads X11/Wayland/GL/ALSA libraries at run time when they're present.
Without the target's dev headers at build time, a cross-built SDL3 gets only the Wayland video backend and no
audio. Install them for the target (Debian multiarch: `libasound2-dev:arm64 libx11-dev:arm64 libdrm-dev:arm64
libgbm-dev:arm64`, `:armhf` for 32-bit) and point the compiler at them, or build natively on the Pi.

## Tested (qemu-user, no hardware yet)
- aarch64 and armv6 (ARMv6/VFPv2 per readelf): ASCII and 2D run 200 frames; peak RSS under qemu (includes ~11 MB
  of qemu) ASCII 36/32 MB, 2D 47/42 MB (aarch64/armv6).
- 3D can't run under qemu-user (no GL); the GLES tiers were validated on x86 with Mesa llvmpipe
  (`BICYCLE_GL=gles3` / `gles2`, `LIBGL_ALWAYS_SOFTWARE=1`): with gles3, test and plat3d match GL 3.3 core, shadows
  included; gles2 renders the same apart from the missing shadows. `MESA_GL_VERSION_OVERRIDE=3.1` (no 3.3 core, like
  v3d) exercises the core -> gles3 fallback with the window recreated.
- plat3d on llvmpipe, ms per frame (shadow pass + main pass; overlay and swap add ~2.5 ms on every tier):
  gles2 0.9 + 3.5, gles3 off 0.9 + 3.2, low 1.5 + 4.0, med 1.8 + 6.8, high 3.5 + 10.9; core high 4.1 + 7.9.
- 32-bit ARM with zig links `src/platform/arm32_libgcc_compat.c` (libgcc names LuaJIT expects from compiler-rt).
