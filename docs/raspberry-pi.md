# Raspberry Pi

| Model | CPU / OS | GPU (Mesa) | ASCII | 2D | 3D |
|---|---|---|---|---|---|
| Pi 4 / 400 | Cortex-A72, 64-bit (aarch64) or 32-bit | VideoCore VI, v3d: GL 3.1 / GLES 3.1 | yes | yes | GLES2 path (no GL 3.3 core), no shadows |
| Pi Zero 2 W | Cortex-A53, 64-bit or 32-bit | VideoCore IV, vc4: GL 2.1 / GLES 2.0 | yes | yes | GLES2 path, no shadows |
| Pi Zero / Zero W | ARM1176 (ARMv6), 32-bit armhf | VideoCore IV, vc4: GLES 2.0 | yes | yes (software framebuffer; slow-ish) | GLES2 path; expect a low frame rate |

## 3D renderers
The 3D mode tries an OpenGL 3.3 core context (GLSL 330, VAOs, shadow map). If that fails it falls back to
OpenGL ES 2.0: the same shaders translated to GLSL ES 1.00, vertex attributes rebound per draw (no VAOs), no shadow
map. Force ES 2.0 with `BICYCLE_GLES2=1` (environment) or `-DBICYCLE_GLES2=ON` (CMake). `BICYCLE_GL_INFO=1` prints
the context and "(GLES2 path)". On a Pi always use the GLES2 path: `BICYCLE_GLES2=1 ./o-3d test`.

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
cmake -B build-3d -G Ninja -DBICYCLE_RENDER=3D -DBICYCLE_GLES2=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build-3d
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
- 3D can't run under qemu-user (no GL); the GLES2 path was validated on x86 with Mesa llvmpipe
  (`BICYCLE_GLES2=1 LIBGL_ALWAYS_SOFTWARE=1`): test and plat3d render like GL 3.3 apart from the missing shadows.
- 32-bit ARM with zig links `src/platform/arm32_libgcc_compat.c` (libgcc names LuaJIT expects from compiler-rt).
