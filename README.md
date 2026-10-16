# bicycle

A game engine with three render modes, LuaJIT game scripts and fluidsynth music and sound effects. The engine is
one executable that runs a game folder: `./o test` runs `test/main.lua`.

| `BICYCLE_RENDER` | executable | draws with | art |
|---|---|---|---|
| `ASCII` (default) | `o` | curses in a terminal (ncurses, PDCurses on Windows) | ASCII exports of Aseprite sprites, text images |
| `2D` | `o-2d` | SDL3 window, software framebuffer | pixel (`.bimg`) exports of the same Aseprite sprites |
| `3D` | `o-3d` | SDL3 + OpenGL 3.3 core | Blender meshes (`model` gene), sprites as billboards, 2D overlay |

The game code is the same in every mode: everything is laid out in character cells (a sprite pixel is 2x1 cells,
as in the ASCII export), so positions, collisions, the camera, the HUD and dialogue don't change. See
`include/v/common/Renderer.h` for the interface each mode implements (`src/v/ascii`, `src/v/2d`, `src/v/3d`, with
`src/v/sdl` shared by the pixel modes).

## Building

Everything the engine needs (LuaJIT, fluidsynth, PDCurses on Windows, and a General MIDI SoundFont) is
downloaded from pinned URLs (checked against SHA-256 hashes) and built from source the first time you configure.
You only need CMake (3.21+), a C/C++ compiler and an internet connection; git isn't needed for the dependencies.

```sh
cmake -B build && cmake --build build
./o test            # o.exe test on Windows
```

Pick the render mode at configure time, one build folder per mode:

```sh
cmake -B build -DBICYCLE_RENDER=ASCII && cmake --build build && ./o test
cmake -B build/2d -DBICYCLE_RENDER=2D && cmake --build build/2d && ./o-2d test
cmake -B build/3d -DBICYCLE_RENDER=3D && cmake --build build/3d && ./o-3d test
```

The pixel modes fetch SDL3 (static). On Linux they need the X11 and/or Wayland development headers
(`libx11-dev libxext-dev`, `libwayland-dev`) and, for 3D, an OpenGL 3.3 driver (Mesa's llvmpipe works). Window
size: `BICYCLE_WINDOW_SIZE=1280x720` (a cell is 8x16 screen pixels). Arrow keys work like WASD.

The first configure takes a minute or two (it builds the dependencies into `build/_deps`); later builds don't
repeat it. The executable lands in the repository root because games and `scripts/` are found relative to the
current folder. Everything is linked statically, so `o`/`o.exe` needs no extra DLLs (any that are used are copied
next to `o.exe` after the build).

`make` is a thin wrapper around the same build (`make clean`, `make install` too). Presets: `cmake --preset default`
(Ninja), `cmake --preset system` (installed packages only), `cmake --preset msvc` (Visual Studio 2022).

### Linux

Needs `cmake`, `g++` (or clang), `make`, and for sound `libasound2-dev` and/or `libpulse-dev`. The system ncurses
is used (`libncurses-dev`); with `-DBICYCLE_FETCH_NCURSES=ON` ncurses is built from source too.

### Windows, MSYS2 CLANG64

In an MSYS2 CLANG64 shell, install just the toolchain, then build:

```sh
pacman -S --needed make mingw-w64-clang-x86_64-{clang,cmake,ninja}
cmake -B build && cmake --build build
./o.exe test
```

(UCRT64 with `mingw-w64-ucrt-x86_64-{gcc,cmake,ninja}` works the same way.) `o.exe` doesn't depend on MSYS2's
DLLs, so it also runs from `cmd` or PowerShell (`o test`, `o.bat test`, `.\o.ps1 test`).

### Windows, Visual Studio

Install Visual Studio 2022 (or the Build Tools) with "Desktop development with C++", which includes CMake. In an
"x64 Native Tools Command Prompt for VS 2022" (LuaJIT's `msvcbuild.bat` needs it), from the repository root:

```bat
cmake --preset msvc && cmake --build --preset msvc
o test
```

### Options

* `BICYCLE_USE_SYSTEM_DEPS` (OFF): use installed packages (pkg-config / CMake config) and download nothing.
* `BICYCLE_FETCH_NCURSES` (OFF): on Linux/macOS, build ncurses instead of using the system one.
* `BICYCLE_DOWNLOAD_GM_SF2` (ON): download GeneralUser GS (32 MB) to `sf2/default-GM.sf2`, the music fallback.
* `BICYCLE_DEPS_DIR` (`build/_deps`): where downloads and dependency builds go.
* `BICYCLE_RENDER` (`ASCII`): `ASCII`, `2D` or `3D`.
* `BICYCLE_GAMES` (`test`): game folders whose assets the `assets` target packs.
* `BICYCLE_RELEASE_ASSETS` (OFF): pack assets every build and load them from the pack (see Assets).

### Music

The General MIDI fallback SoundFont is looked up at `BICYCLE_GM_SF2`, then `./sf2/default-GM.sf2` (downloaded by
the build), then the usual Linux system paths. See `tools/audio/README.md`.

### How Windows differs

* Keyboard: the console's `_kbhit()`/`_getch()` instead of a raw (termios) terminal.
* Terminal resizes: `GetConsoleScreenBufferInfo()` instead of `ioctl(TIOCGWINSZ)`.
* Curses: PDCurses (wincon port) instead of ncurses, both through `include/curses_compat.h`.
* Lua calls the engine with LuaJIT's FFI (`ffi.C.*`), which finds functions among the executable's exports.
  CMake exports them (`ENABLE_EXPORTS` and `WINDOWS_EXPORT_ALL_SYMBOLS`; `--export-all-symbols` on MinGW).
  The FFI declarations in `scripts/bicycle.lua` only use fixed-size types, so they're the same on both.
* `scripts/Scene.lua` lists folders with `dir /b` instead of `ls`.

### Cross-compiling o.exe from Linux (a check, not a release build)

`cmake/mingw-w64.cmake` is a MinGW-w64 toolchain file; the dependencies are fetched and cross-built as above.

```sh
cmake -B build/mingw -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake && cmake --build build/mingw
# llvm-mingw (clang) instead of gcc:
cmake -B build/clang64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake \
      -DMINGW_CC=x86_64-w64-mingw32-clang -DMINGW_CXX=x86_64-w64-mingw32-clang++ && cmake --build build/clang64
```

Add `-DBICYCLE_RENDER=2D` or `3D` for `o-2d.exe` / `o-3d.exe`. The resulting executables run under Wine.

## Assets

### Dev workflow vs finalizing a game

**Dev (the default)**: the engine reads loose, uncompressed files straight from the game folder: Lua and genomes,
Aseprite `.bimg` exports, `.mid`/`.sf2`, and meshes as the `.obj`/`.mtl` the Blender exporter writes (parsed at load,
`src/data/Obj.cpp`). Edit a file and rerun `./o test`: there is no pack and no compile step, and no pack is mounted
even if an old `assets.bpak` is lying around.

**Release / finalizing**: configure with `-DBICYCLE_RELEASE_ASSETS=ON`; then every build runs the `assets` target and
the engine mounts `<game>/assets.bpak` and reads through it. `cmake --build build --target package` packs a game
in either mode. Both paths read into the same memory pools (see below); only the source of the bytes differs.

### Packing and compression

The `assets`/`package` target (`tools/pack`, a host tool, also when cross-compiling) compresses each game folder's
`main.lua` and `genome/**` into `<game>/assets.bpak`. A release build mounts the pack at start-up and reads
everything through it (`include/data/Pack.h`): Lua modules (a `package.loaders` searcher), Aseprite exports, MIDI,
SoundFonts, images and meshes. Files missing from the pack are read from disk, and `BICYCLE_LOOSE_ASSETS=1`
ignores the pack.

| entry | stored as | decoded with |
|---|---|---|
| `.lua`, `.txt`, `.mid`, `.sf2`, ... | zlib (miniz at level 10 when packing) | jollybean's `tinfl` (`src/data/tinfl.c`) |
| `.bimg` (indexed) | jollybean's strip codec (deduplicated runs of N pixels, bit-packed indices, deflated), or deflate if smaller | `src/data/Strip.cpp` |
| `.obj` (Blender) | `.bmesh`: Edgebreaker connectivity, quantized + parallelogram-predicted positions, octahedral normals, delta UVs, all deflated | `src/data/Mesh.cpp`, `src/data/Edgebreaker.cpp` |

Game SoundFonts are handed to fluidsynth from memory (a custom SoundFont loader whose file callbacks read a
buffer; names start with `bicycle-mem://`), and MIDI files with `fluid_player_add_mem`, so neither touches the
disk. The 32 MB General MIDI fallback (`sf2/default-GM.sf2`) stays a plain file outside the pack: it's shared by
every game and barely compresses (SoundFont samples are PCM).

`ctest` (or `build/bicycle-tests test`) round-trips every codec and every pack entry, including the Blender meshes.

### 2D art: Aseprite

`scripts/aseprite/sprite.lua` exports an `.aseprite` file to `test/genome/` (see `scripts/aseprite/README.txt`):

```sh
aseprite -b test/aseprite/pilgrim.aseprite --script scripts/aseprite/sprite.lua
```

`BICYCLE_EXPORT=ascii|pixels|both` (default `both`) picks the outputs. ASCII writes `<layer>_colormap.lua` (ASCII
art) and per-tile ASCII; pixels writes `<layer>_pixels.bimg` (an indexed frame strip, frame f at x = (f-1) * canvas
width, index 0 transparent) and `<layer>_tileset_pixels.bimg`, and records each layer's palette in
`<layer>_palette.lua`. Animation src rects, tags, tilemaps, background modes and collision data are shared by both.
The pixel modes use the `.bimg` files when a sprite has them, and a genome can name different art for them with
`sprite2d` (e.g. the horse keeps its hand-drawn ASCII `img` and uses `horse_sprite` in 2D/3D).

### 3D art: Blender

See `scripts/blender/README.md`: `make_scene.py` authors the test scene headless, `export.py` exports it
(`blender -b scene.blend --python scripts/blender/export.py -- test/genome/models`), and the pack turns the OBJs
into `.bmesh`. Genomes pick a mesh with `model = "<object>"`.

## UI per render mode

`src/v/common` (Window, Dialogue, TextMenu, Bar) lays UI out in cells and calls the renderer; each mode draws it
its own way behind `gfx::Renderer` (`drawBar`, `drawMenuCursor`, `setPanel` + `clearWindow`/`drawBorder`). ASCII
keeps curses boxes and the text bar. 2D (`src/v/2d`) draws flat pixel-art panels and a segmented bar; 3D
(`src/v/3d`) draws translucent gradient panels and a glossy pill bar in the overlay. Both color the bar by
fraction (green, then yellow/amber, then red), take optional bar sprites (`hp:sprites( frame, fill )` in
`scripts/Hud.lua`), and render text with TrueType (`fonts/ui.ttf`, stb_truetype). In the pixel modes, ASCII-art
images that have no pixel version are drawn as solid cell blocks, not characters.

## Texture atlas (2D and 3D)

Every sprite and tileset image is registered as it loads and packed into one atlas (`src/v/sdl/SpriteAtlas.cpp`)
with jollybean's planner (`src/data/Atlas.cpp`, ported from `xRender.c`: sort by largest side, binary tree of
right/lower placements, grow right or down). The atlas is re-planned once after a batch of loads, not per frame;
its ARGB pixels live in GRAPHIC. 2D reads sprite pixels from it; 3D uploads it as one texture and draws all
billboards in a single batched draw call. Mesh textures (which tile with GL_REPEAT) keep their own textures.
`BICYCLE_ATLAS_INFO=1` prints the atlas size on each rebuild.

## Memory pools

All engine memory comes from one arena cut into bump-allocated pools (`include/data/Mem.h`, after jollybean's
`mem.c`): PERMANENT (asset pack), GENERAL (every `new` in the engine, std containers), TEXT (the LuaJIT heap and
scripts), GRAPHIC (images, meshes, textures, framebuffers, GL staging), AUDIO (SoundFonts, MIDI) and TEMPORARY
(scratch, reset every frame and after each image load). `mem::Allocator` / `mem::Vector` / `mem::String` /
`mem::Resource` (pmr) put std containers in a pool. Sizes: `-DBICYCLE_MEM_<POOL>_MB=` or the same environment
variable; overflowing a pool is fatal and names it. `BICYCLE_MEM_STATS=1` prints high-water marks at exit.

Limits: LuaJIT takes the custom allocator only in GC64 builds (the default on x64/arm64, which the fetched LuaJIT
is); a non-GC64 build falls back to LuaJIT's own allocator with a warning. fluidsynth copies SoundFont samples into
its own malloc'd memory, so only the sf2/mid bytes are in AUDIO. Third-party C code (SDL, fluidsynth, GL drivers)
still uses malloc. fluidsynth's cpp11 thread code is patched at fetch time (new vs free mismatch).

## Environment variables

`BICYCLE_LOOSE_ASSETS`, `BICYCLE_WINDOW_SIZE`, `BICYCLE_SCREENSHOT=<file.bmp>` (+ `BICYCLE_SCREENSHOT_FRAME`,
default 90, and `BICYCLE_EXIT_AFTER_SCREENSHOT=1`) and `BICYCLE_GL_INFO` for the pixel modes;

and `BICYCLE_AUDIO_DRIVER`, `BICYCLE_AUDIO_FILE`, `BICYCLE_GM_SF2`, `BICYCLE_MUTE_MUSIC`, `BICYCLE_PROFILE` (audio
start-up timings) and `BICYCLE_DEBUG_LOG` (Lua `dbglog`): see `include/c/MidiPlayer.h` and `tools/audio/README.md`.
