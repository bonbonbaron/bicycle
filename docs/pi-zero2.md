# Raspberry Pi Zero 2 W (64-bit Raspberry Pi OS)

Cross-build from x86_64 Linux with zig 0.14 (targets glibc 2.36 = bookworm; static libc++, needs only libc/libm):

```sh
# wrappers: exec zig cc|c++ -target aarch64-linux-gnu.2.36 -mcpu=cortex_a53 "$@"  (and zig ar, zig ranlib)
export ZIG_WRAP=$HOME/zigwrap     # a64-cc a64-c++ a64-ar a64-ranlib
cmake -B build-pi-2D -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-zig.cmake -DBICYCLE_RENDER=2D -DCMAKE_BUILD_TYPE=Release
cmake --build build-pi-2D          # ASCII: add -DBICYCLE_FETCH_NCURSES=ON
```

Status (qemu-aarch64; not yet run on hardware):
- ASCII and 2D run. 2D draws into a software framebuffer and presents through SDL, so GLES 2 (vc4) is enough.
- 3D needs OpenGL 3.3 core (#version 330 shaders, VAOs, depth-texture shadow map with sampler2DShadow).
  The Zero 2's VideoCore IV (Mesa vc4) offers GL 2.1 / GLES 2.0 only, so 3D only runs on Mesa's llvmpipe
  software rasterizer (LIBGL_ALWAYS_SOFTWARE=1), which is very slow on a Cortex-A53.
- Without arm64 X11/libdrm/ALSA headers at build time, SDL3 gets only its Wayland video backend (fine on
  bookworm's labwc/wayfire desktop, not on X11 or console/KMS) and no audio backend (silent).
  Install them for the target (e.g. libx11-dev libdrm-dev libgbm-dev libasound2-dev for arm64) to get those.
- armhf (32-bit OS) is not built: LuaJIT's buildvm must then be built as a 32-bit host program (gcc -m32).
