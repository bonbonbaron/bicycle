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
cmake --preset windows-clang64 && cmake --build --preset windows-clang64
./o.exe test
```

The preset builds RelWithDebInfo into `build/clang64` with clang and Ninja (debug info split into `o.exe.debug`);
add `-DCMAKE_BUILD_TYPE=Debug` for -O0 stepping or `-DBICYCLE_RELEASE=ON` for a release. `make` is needed only for
LuaJIT's own Makefile on the first configure.

(UCRT64 with `mingw-w64-ucrt-x86_64-{gcc,cmake,ninja}` works the same way, with `--preset windows-ucrt64`.) `o.exe` doesn't depend on MSYS2's
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
* `BICYCLE_RELEASE` (OFF): a shippable build with **zero file I/O at run time**: the game, the engine's Lua
  scripts and fonts are linked into the executable (see "Release builds: everything from memory"). Implies
  `BICYCLE_RELEASE_ASSETS`. Needs GCC/clang (MinGW, llvm-mingw/CLANG64 or Linux), not MSVC.
* `BICYCLE_EMBED_GM_SF2` (OFF): with `BICYCLE_RELEASE`, also embed `sf2/default-GM.sf2` (+30 MB); otherwise a
  release build has no General MIDI fallback, only the game's own SoundFonts.
* `BICYCLE_TERMINFO_FALLBACKS`: with `BICYCLE_RELEASE` (ASCII, Linux/macOS), the terminals whose terminfo is
  compiled into ncurses (default: xterm(-256color), screen, tmux, linux, vt100, rxvt-unicode, st, alacritty,
  kitty, foot, wezterm; whichever this machine's terminfo has).

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

### Genome layout

Every game keeps its data in `<game>/genome/`, sorted by kind (`scripts/GenomeLayout.lua` is the one place the
engine and the Aseprite exporters get these paths from):

```
<game>/genome/
  <entity>.lua                 genomes (hero.lua, coin.lua, door.lua, redbeetle.lua, ...) and the start genome
  body/<sprite>.lua            sprite manifests (layers, animation): what a genome's sprite = "..." names
  body/palette/color/          *_palette.lua      gray index -> color (one per variant: green beetle, red beetle)
  body/palette/collision/      *_collset.lua      collision index -> collision type (normal, fire, ...)
  body/graybody/               *_pixels.bimg      palette-agnostic gray (indexed) pixels: sprites and tilesets
  body/graybody/animation/     *_anim_src_rects.lua, *_globalAnim.lua   frames, tags, durations
  body/graybody/collision/     *_collmap.lua, *_collRects.lua           collision regions / masks
  body/graybody/colormap/      *_colormap.lua, *.txt                    ASCII art (and img gene text images)
  tilemap/                     *_tilemap.lua, *_tileset.lua             background tile layouts and tilesets
  scene/                       scene tables (scene/bg/<name>/: text backgrounds)
  body/model/                  genome models: meshes a genome's model = "..." names (.obj/.mtl -> .bmesh, .bskin)
  level/                       3D levels from Blender (tools/blender): meshes, placed genomes, spawn points
  model/                       level meshes (.obj/.mtl, packed .bmesh) and mesh textures
  music/                       songs (.mid) and SoundFonts (.sf2)
```

**Asset references are bare names.** A genome or scene names an asset by its file name alone, never a folder or
an extension; the field says what kind it is and `scripts/GenomeLayout.lua` (`Layout.REFS`) says where that kind
lives: `soundfont = "meadow"` is `genome/music/meadow.sf2`, `song = "house"` is `genome/music/house.mid`,
`model = "pilgrim3d"` is `genome/body/model/pilgrim3d` (.bmesh in a pack, the loose .obj in dev),
`level = "courtyard"` is `genome/level/courtyard.lua`, `sprite = "goldcoin"` is `genome/body/goldcoin.lua`. The
ID generator (below) finds every such file, so a typo'd or missing name is a build error that lists what the game
has, and a name two files share (`music/house.mid` and `music/more/house.mid`) is reported as ambiguous. Old
paths (`"music/meadow.sf2"`) still work.

### Self-contained genomes

A genome (`genome/<name>.lua`) is everything about one kind of thing, so it can be dropped into any scene as is:
its sprite or model, animation, collision, motion, sounds, timer, behavior and the parameters each placed copy
may change. A scene only says where its copies go. plat3d's coin:

```lua
local coin = {
  model = "coin3d",                                         -- genome/body/model/coin3d
  body = { kind = "kinematic", radius = 0.4, height = 0.8, pitch = 90 },
  coll = { type = "coin" },
  motion = { type = "spin", speed = 180, bob = 0.12, bobHz = 0.8 },     -- the engine spins and bobs it
  sfx = { soundfont = "sfx", program = 120, sounds = { collect = 72 } }, -- its own sounds, named its own way
}
function coin:onCollision( other, collType )
  if collType ~= 1 then return end                          -- Genome.COLL.hero
  self:sound( "collect", 115 )
  entityOf( other ):collect( self )
  require( "Scene" ).removeForever( self )                  -- stays collected when the hero comes back
end
return coin
```

* **Sounds** (`sfx` gene): `soundfont`, `program` (and `bank`), and `sounds = { <name> = <MIDI key> | { key,
  program, soundfont, bank, vel } }`. Names are the genome's own: two genomes can both have a "hit".
  `self:sound( "collect" [, vel] )` plays one (where the body is, in 3D). A SoundFont several genomes use is loaded
  once (`Sound.soundfonts`, outside the scene's memory scope). Scenes have no sound-effect lists any more (an old
  `scene.sfx` still loads, with a warning). Samples go into a SoundFont (`tools/audio/make_sf2.py`).
* **Behavior** is events only: `onSpawn`, `onInput( key, pressed, dx, dy )` (see "Input" below),
  `onCollision( other, type )`, `onUncollision`, `onTimer( timerId, type )`, `onSceneEnter( scene, spawn )`,
  `onDespawn`; in 3D also `onLand( self )`, `onFallOut( self )` (below the genome's `killY`, default -15) and
  `onLevelReady( self )` (everything the level places exists). `listen = true` (3D genomes; `listenInput( id )` for
  anything) makes it get every input event without having the focus (a camera genome). `timer = <ms> | { ms, rep, type }`
  starts a timer at spawn; `self:after( ms, type )`, `self:every( ms, type )` and `self:stopTimer( id )` add more.
  Continuous motion is data (`motion`: linear paths, orbits, spin and bob, follow; or an animation), moved by the
  engine's Kinematics, not per-frame Lua.
* **Per-instance parameters**: whatever a placement adds (a door's `door = { to = "bonus" }`, a mover's
  `motion = { path = ... }`) is merged over the genome's own fields (nested tables merge, lists replace).
  `params = { ["door.to"] = "scene", ["door.spawn"] = "spawn:door.to", ["motion.path"] = "path", ... }` tells the
  Blender add-on which ones to offer, and how (a scene drop-down, the spawn points of that scene, path points, a
  number...): see [docs/blender-entities.md](docs/blender-entities.md).

**Placing them.** 2D scenes: on the background sprite in Aseprite (below). 3D scenes: in Blender with the add-on in
`tools/blender` (genome drop-down, door and spawn drop-downs, path points, spawn points); the exporter writes
`genome/level/<scene>.lua` and the engine spawns its genomes when the scene loads. A 3D scene file is then only
its look: `world3d = { level = "courtyard", sky, fog, light }` (and `music`).

New game: `scripts/new_game.sh mygame` (or by hand: `mkdir -p mygame/genome/body/palette/{color,collision}
mygame/genome/body/graybody/{animation,collision,colormap} mygame/genome/{tilemap,scene,level,model,body/model,music}`).

**Graybody + palettes (palette swaps).** The graybody is shared by every variant of a body: its gray pixels,
animation and collision regions are loaded once (`Sprite.load` caches it in Lua; `newPixelImage` shares one
image per `.bimg` among all entities). A genome picks the palettes separately:

```lua
local redbeetle = { sprite = "beetle_sprite",           -- graybody (+ its animation) shared with genome/beetle.lua
                    palette = "redbeetle_palette",      -- body/palette/color/: { colors = { [1] = 0xFF2828BE, ... } }
                    collpal = "redbeetle_collset",      -- body/palette/collision/: { types = { [1] = "fire" } }
                    coll = {} }                         -- no type: the collision palette picks it (region 1)
```

A color palette lists only the gray indices it recolors (RGBA, R in the low byte, as in `.bimg`); an exporter
`*_palette.lua` works too. The variant image keeps a pointer to the graybody's indices (`BImg::gray`) instead of
copying them, and every red beetle shares that one variant. Like a green and a red koopa: same graybody, same
animation, different color and collision palettes.

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
disk. The 32 MB General MIDI fallback (`sf2/default-GM.sf2`) stays a plain file outside the pack in dev and
`BICYCLE_RELEASE_ASSETS` builds: it's shared by every game and barely compresses (SoundFont samples are PCM). A
`BICYCLE_RELEASE` build embeds it only with `BICYCLE_EMBED_GM_SF2`.

`ctest` (or `build/bicycle-tests test`) round-trips every codec and every pack entry, including the Blender meshes.

### Release builds: everything from memory

`-DBICYCLE_RELEASE=ON` makes an executable that reads no files at all once `main()` starts:

```sh
cmake -B build-rel -G Ninja -DCMAKE_BUILD_TYPE=Release -DBICYCLE_RELEASE=ON [-DBICYCLE_RENDER=2D|3D] && cmake --build build-rel
./o            # runs the embedded game (the first of BICYCLE_GAMES); `./o test` also works, nothing else is needed
ctest --test-dir build-rel -R release-zero-io   # proves it with strace (below)
```

**What's embedded.** The build packs the game (`main.lua`, `genome/**`: genomes, Aseprite exports, tilemaps, MIDI,
SoundFonts, Blender meshes and skins) together with `scripts/*.lua` (as `scripts/<name>.lua`) and the UI fonts
(`<game>/fonts/*.ttf` if the game has any, else `fonts/*.ttf`, as `fonts/<name>.ttf`), plus with
`BICYCLE_EMBED_GM_SF2` the GM SoundFont as `sf2/default-GM.sf2`, into `build-rel/embedded-<game>.bpak` (same
compression as above). `src/data/EmbeddedPack.c` links it in with `.incbin` (ELF and COFF, so Linux, MinGW and
llvm-mingw alike), and the engine mounts those bytes in place: no copy, no file. Shaders are string literals in
`src/v/3d/Renderer3D.cpp`. The 2D/3D pixel modes' bitmap fallback font is compiled in (`Font8x8.h`). The `test` game
embeds 1.1 MB.

**How each subsystem loads** (`BICYCLE_RELEASE` is a compile-time switch; the file paths are compiled out):

* Asset reads (`assets::read/exists/list`, `include/data/Pack.h`): only the embedded pack; `readFile` and the
  loose-file fallback are compiled out (no `<fstream>`).
* Lua: `package.loaders` is preload + the pack searcher (`require("Genome")` finds `scripts/Genome.lua`, a game's
  `require("./test/genome/hero")` its genome); Lua's file searchers are removed and `package.path`/`cpath` are empty.
  `io.open`, `io.popen`, `os.remove/rename/execute`, `package.loadlib` and `loadfile` return nil + "no file I/O";
  `dofile`, `io.lines/input/output`, `os.tmpname` raise. `io.write`/`io.stderr` still work. Scripts load assets with
  `Asset.read/dofile/list` (`scripts/bicycle.lua`), which go through the pack.
* Audio: SoundFonts are inflated into the AUDIO pool and loaded through fluidsynth's SoundFont loader with memory
  callbacks (`fluid_sfloader_set_callbacks`; `bicycle-mem://` names); the callbacks' `fopen` branch is compiled out.
  MIDI goes through `fluid_player_add_mem`. The GM fallback comes only from the pack. `BICYCLE_AUDIO_FILE` and
  `BICYCLE_PROFILE` (they write files) are compiled out.
* Fonts: `UiFont` reads `fonts/ui.ttf` through the pack. Screenshots (`BICYCLE_SCREENSHOT`) are compiled out.
* ASCII mode: ncurses is built from source with `--disable-database --with-fallbacks=...`, so terminal descriptions
  are compiled in and no terminfo file is read (`TERM` must be one of `BICYCLE_TERMINFO_FALLBACKS`). Windows uses
  PDCurses, which reads no files. `setlocale` asks only for `LC_CTYPE` (UTF-8).
* 2D/3D on Linux: `MESA_SHADER_CACHE_DISABLE=true` is set before SDL starts (unless already set), so mesa doesn't read
  or write `~/.cache/mesa_shader_cache`.

**The check.** `tests/release_io_test.sh` (CTest `release-zero-io`, Linux, needs `strace`, and `xvfb-run` for
2D/3D without a display) copies the executable into an empty temporary folder, runs it headless for 120 frames
(`BICYCLE_MAX_FRAMES`) under `strace -f -e trace=open,openat,openat2,creat,stat,lstat,newfstatat,statx,access,
faccessat(2),readlink(at)`, and looks at every path touched after `main()` starts (marked by a `prctl(PR_GET_NAME)`
in `src/main.cpp`; the dynamic loader's opens come before). It fails on anything that isn't the operating system's:
any relative path, the game folder, `scripts/`, `fonts/`, `sf2/`, a `.bpak`, config or save files. What remains,
and is allowed, is the OS's own doing:

| mode | remaining opens | why |
|---|---|---|
| all | `/dev/snd/*`, `/usr/share/alsa/**`, `/etc/alsa/conf.d/*`, `~/.asoundrc` (probe) | fluidsynth's ALSA driver: alsa-lib reads its config and probes sound cards |
| all | `bicycle-mem://test/genome/music/meadow.sf2` (stat, ENOENT) | fluidsynth stats a SoundFont name before calling our loader; not a file, nothing read |
| ASCII | `/usr/lib/locale/C.utf8/LC_CTYPE`, `locale.alias`, `gconv-modules.cache` | `setlocale(LC_CTYPE, "")` for UTF-8 output |
| 2D/3D | `/lib/x86_64-linux-gnu/lib{GL,GLX_mesa,gallium,LLVM,drm,X*,xcb-*,wayland-*,decor}...`, `/etc/ld.so.cache` | SDL and libglvnd `dlopen` the window system and GL driver at run time |
| 2D/3D | `/usr/lib/locale/**`, `/usr/share/X11/locale/**`, `~/.XCompose`, `~/.compose-cache`, `/var/cache/libx11/compose`, `~/.Xdefaults*`, `~/.Xauthority` | X11: SDL's `XOpenIM` (keyboard input method, compose tables), Xlib resources and auth |
| 2D/3D | `/usr/share/icons/**/cursors/*`, `~/.icons/**`, `/usr/share/pixmaps/**` | libXcursor loading the cursor theme for SDL's system cursors |
| 2D/3D | `/usr/share/drirc.d/*`, `/etc/drirc`, `~/.drirc`, `/dev/dri/*`, `/dev/udmabuf`, `/sys/devices/system/cpu/*`, `/proc/self/exe` | mesa: driver config (matched against the executable's name), GPU devices, CPU count |

On Windows the same build (PDCurses, WinMM/DirectSound, WGL) reads no files of its own either; the OS loads its
DLLs. The `strace` check is Linux-only.

### 2D art: Aseprite

`scripts/aseprite/sprite.lua` exports an `.aseprite` file into `test/genome/`'s layout (Genome layout above; see
`scripts/aseprite/README.txt`):

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

Install the add-on `tools/blender/bicycle_blender.py` (Edit > Preferences > Add-ons > Install from Disk) and use the
Bicycle tab of the 3D Viewport's sidebar to place genomes and export levels: [docs/blender-entities.md](docs/blender-entities.md)
walks through it click by click. Headless, `scripts/blender/export.py` does the same export
(`blender -b plat3d/blender/plat3d.blend --python scripts/blender/export.py`); see `scripts/blender/README.md`. The pack
turns the OBJs into `.bmesh`. Genomes pick a mesh with `model = "<object>"` (genome/body/model/).

### Scene tools in Aseprite: doors and music

`scripts/aseprite/adddoor.lua` adds a door to a scene. Open the scene's background sprite (the game and scene
come from its path: `test/aseprite/meadow.aseprite` is `test`'s `intro`, whose `bg = "meadow"`), select the
doorway with the marquee (or add a slice named `door`), run File > Scripts > adddoor, and pick the target scene
and its spawn (both drop-downs list what's in `<game>/genome/scene/`; the spawn list follows the scene). It
writes `{ genome = "door", pos = {...}, door = { to, spawn, w, h, on } }` into the scene's first fg layer and can
add the return spawn here (`from_<target>`, beside the door). `scripts/aseprite/addsong.lua` sets a scene's
`music = { soundfont = "meadow", song = "house", transition }` (bare names) from the game's `genome/music/*.mid` / `*.sf2` (transition:
crossfade, cut or keep, used when the scene is entered unless `Scene.go` says otherwise).

Batch mode (`--script-param list=1` prints the drop-downs' choices):

```sh
aseprite -b --script-param game=test --script-param scene=house --script-param to=intro \
  --script-param spawn=from_house --script-param x=140 --script-param y=36 --script-param w=6 --script-param h=10 \
  --script-param on=press --script-param return_spawn=from_intro --script scripts/aseprite/adddoor.lua
aseprite -b --script-param game=test --script-param scene=house --script-param song=house \
  --script-param transition=cut --script scripts/aseprite/addsong.lua
```

### Placing entities in Aseprite

A scene's entities live on its background sprite, on a layer named `entities` (`test/aseprite/meadow.aseprite`
for `intro`, `house.aseprite` for `house`). Each entity is a slice named `<genome>#<n>` whose top-left is its
position (1 sprite pixel = 2x1 cells) and whose properties hold `genome`, `fg` (the scene's fg layer), `order` (spawn
order) and `extra` (other fields as Lua, e.g. `focus = true, persist = true` or `door = { to = "house", spawn =
"entry" }`). The entities layer draws an outline for each so you can see them; it's never exported as art.

* `scripts/aseprite/addentity.lua`: select a spot, pick a genome (the game's `genome/*.lua`, the same names as
  `ids.genome`), the fg layer, persist/focus, a motion preset (linear, orbital, combo) and any more fields.
* `scripts/aseprite/adddoor.lua` with the scene's background open makes the door a slice there too (its size is
  the door's `w`/`h`); without it, it edits the scene file as before.
* Move or resize slices in Aseprite, then re-run either script (`--script-param sync=1`) or export with
  `sprite.lua`: the scene file's entities are rewritten.

They're written into `genome/scene/<scene>.lua` between `-- BEGIN entities (generated ...)` and `-- END entities`
in each fg layer; entries you write by hand outside those markers are kept, and re-exporting gives the same text.
`--script-param import=1` moves a scene's hand-written entries onto slices once (that's how intro and house were
converted, with identical output). Batch:

```sh
aseprite -b test/aseprite/meadow.aseprite --script-param genome=coin --script-param x=30 --script-param y=42 \
  [--script-param fg=4] [--script-param persist=1] [--script-param motion=orbital] --script scripts/aseprite/addentity.lua
aseprite -b test/aseprite/meadow.aseprite --script-param list=1 --script scripts/aseprite/addentity.lua
```

plat3d and fps3d place their entities in Blender instead (docs/blender-entities.md).

`ctest -R aseprite-scene-scripts` runs both on a copy of the test game (configure with
`-DBICYCLE_ASEPRITE=/path/to/aseprite`).

## Motion and tracking (Kinematics)

Kinematics drives four motion types (`include/m/Motion.h`; the math is in `include/c/MotionMath.h` and is
unit-tested by `tests/motion_test.cpp`). Units are cells (a sprite pixel is 2x1 cells) and timing is framerate
independent.

* `linear`: walk waypoints at `speed` cells/s, looping or ping-ponging.
* `orbital`: circle or ellipse around a point or an entity (`radius` in rows, so rx = 2*radius; `rx`/`ry` to override;
  `speed` in deg/s; `phase`).
* `combo`: an orbit whose center travels a waypoint path (`travel` cells/s).
* `follow`: `pos += (target+offset-pos)*(1-exp(-rate*dt))`, with an optional dead zone, offset mirroring and idle
  bob. `mode="hard"` (rate 0) snaps every frame.

A genome declares one with the `motion` gene; motions start once every entity in the scene has spawned:

```lua
motion = { type = "follow", target = "hero", mode = "lerp", rate = 3.5, offset = {9, -5}, bob = 0.6, bobHz = 1.1 }
motion = { type = "orbital", around = "villager", radius = 5, speed = 100 }
motion = { type = "combo", radius = 3, speed = 200, travel = 5, path = {{0,0},{50,0}}, pingpong = true }
motion = { type = "linear", speed = 6, path = {{0,0},{36,0},{36,3},{0,3}} }
```

At runtime, `track(follower, target, opts)` makes any entity a follower, with
`opts = {mode="lerp"|"hard", rate=8, offset={x,y}, deadzone={x,y}, mirror, bob, bobHz}`. Calling it again switches
mode and keeps the current position.

### The camera is a kinematic body

The camera has no follow logic of its own. It is an entity that Kinematics moves with the same follow motion, clamped
to the world bounds. Its position is sub-cell (fixed point). `Camera.track` takes the same options as `track`:

```lua
Camera.track(hero.id, { mode = "lerp", rate = 5, deadzone = {10, 3} })  -- smooth
Camera.track(hero.id, { mode = "hard" })                                -- snap to the hero each frame
```

The 2D renderer offsets each layer and sprite by the camera's fractional cell in screen pixels, so scrolling moves
in 1-px steps even when a logical pixel is 16 px wide. The framebuffer is drawn on the CPU, so there are no seams,
and the UI stays fixed. 3D reads the float position. ASCII snaps to whole cells. In the test game,
`BICYCLE_CAMERA=hard` picks hard tracking.

## UI per render mode

`src/v/common` (Window, Dialogue, TextMenu, Bar) lays UI out in cells and calls the renderer; each mode draws it
its own way behind `gfx::Renderer` (`drawBar`, `drawMenuCursor`, `setPanel` + `clearWindow`/`drawBorder`). ASCII
keeps curses boxes and the text bar. 2D (`src/v/2d`) draws flat pixel-art panels and a segmented bar; 3D
(`src/v/3d`) draws translucent gradient panels and a glossy pill bar in the overlay. Both color the bar by
fraction (green, then yellow/amber, then red), take optional bar sprites (`hp:sprites( frame, fill )` in
`scripts/Hud.lua`), and render text with TrueType (`fonts/ui.ttf`, stb_truetype). In the pixel modes, ASCII-art
images that have no pixel version are drawn as solid cell blocks, not characters.

HUD text labels: `local t = Hud.text( text, ax, ay )` with `ax` = `"left"`, `"right"`, `"center"` or a cell x and
`ay` = `"top"`, `"bottom"` or a cell y; `t:set( text )` changes it ("right" keeps its right edge at the screen's
edge). Like bars they come back after a scene change (`Hud.rebuild`) until `Hud.forget( t )`. ASCII prints plain
text; 2D and 3D draw it on the bars' plate (`drawHudText`). `fps()` (C `frameRate()`) is the main loop's frame
rate over about half a second, in every mode. Update labels from `onTimer`, e.g. plat3d's hero:
`self.fpsLabel = Hud.text( "", "right", "top" )`, `self:every( 500, FPS_TIMER )`, and in `onTimer`
`self.fpsLabel:set( string.format( "%.0f fps", fps() ) )`.

ASCII follows the terminal's size. Each frame the renderer polls it: `TIOCGWINSZ` on Linux/macOS, PDCurses'
`is_termresized()` on Windows. On a change it resizes curses (`resize_term`) and tells every window
(`Window::onTermResize`):
* The scene refits its view.
* The camera's limits and dead zone are recomputed and the camera is clamped.
* A bottom dialogue stays full width at the bottom and rewraps its text.
* Other windows are pulled back on screen.

Curses' screen cache is invalidated, so the next frame repaints every cell. Below 32x10 the game shows a "too
small" notice until the terminal grows again.

## Texture atlas (2D and 3D)

Every sprite and tileset image is registered as it loads and packed into one atlas (`src/v/sdl/SpriteAtlas.cpp`)
with jollybean's planner (`src/data/Atlas.cpp`, ported from `xRender.c`: sort by largest side, binary tree of
right/lower placements, grow right or down). The atlas is re-planned once after a batch of loads, not per frame;
its ARGB pixels live in GRAPHIC. 2D reads sprite pixels from it; 3D uploads it as one texture and draws all
billboards in a single batched draw call. Mesh textures (which tile with GL_REPEAT) keep their own textures.
`BICYCLE_ATLAS_INFO=1` prints the atlas size on each rebuild.

## Skeletal animation (3D)

Rigged models export a `.bskin` next to their mesh (see `scripts/blender/README.md`); the 3D renderer skins them on
the GPU and `anim:set(tag)` chooses the action (`walk_left` -> `walk`). Unrigged models still turn and bob.

## Scene changes

`Scene.go( name, opts )` (also `Scene.goTo`, or `Scene["goto"]`: `goto` is reserved in LuaJIT) leaves the
current scene for `genome/scene/<name>.lua` at the end of the frame:

```lua
local Scene = require('Scene')
Scene.go( "house", { spawn = "entry", transition = "fade", duration = 0.6, music = "crossfade" } )
-- transition: "fade" | "wipe" | "dissolve" | "none" (default: dissolve in ASCII, fade in 2D/3D)
-- music: "crossfade" (fade out, then the new song fades in; a scene with the same song keeps playing) | "cut" | "keep"
```

A scene names its arrival points and its doors (the `door` gene: an invisible collision trigger):

```lua
house.spawns = { entry = { 20, 40 } }
house.fg = { [2] = {
  { genome = "door", pos = { 2, 36 }, door = { to = "intro", spawn = "from_house", w = 8, h = 12 } },
  { genome = "keepsake", pos = { 120, 42 }, key = "keepsake" },
} }
```

`door = { to, spawn, w, h (cells), on = "touch" | "press", transition, duration, music }`: "press" doors wait for
the player to call `Scene.useDoor()` (the test hero does on E/Space). A door touched within half a second of
arriving waits until the player steps off it, so spawning on a door doesn't bounce you back.

Genomes with `persist = true` (the hero, the fairy) are carried over: same entity, same Lua table, same timers. The
player lands on the spawn point, companions keep their offset from it (followers snap there instead of flying
across the map), the camera keeps tracking with the same options, the HUD is rebuilt, and each gets
`onSceneEnter( sceneName, spawnName )`. Everything else is despawned (`onDespawn` first).

The order is fixed and runs between frames: 1. the transition covers the screen (input is frozen); 2. windows over
the scene, then the scene's entities, are despawned; 3. the scene window pops and the SCENE memory pool is reset;
4. the new scene loads; 5. the persistent entities arrive; 6. the transition uncovers it. A `Scene.go` during a
change is ignored (returns false) unless `replace = true` and the screen isn't covered yet.

Per-scene saved state: `Scene.state` is a Lua table kept per scene file for the whole run (put anything in it).
`Scene.removeForever( self )` despawns an entity at the end of the frame and keeps it out of that scene from then
on (instances are identified by `key`, or genome + position). `Scene.preload( name )` reads the next scene's Lua
ahead of time.

Memory: everything a scene loads (images, tilemaps, meshes, songs) goes into the SCENE pool
(`BICYCLE_MEM_SCENE_MB`, default 64), which is reset on every change, only once nothing in it is still alive, so it
never leaves a dangling pointer; anything left is reported. SoundFonts and persistent entities stay in the
long-lived pools. `BICYCLE_SCENE_STATS=1` prints the pools on each change; `ctest -R scene-lifecycle` walks back and
forth 20 times and fails if the SCENE pool isn't empty after each unload or its high-water mark grows.

Release builds: the packer checks that every scene a door, `W.door{ to = ... }` or `Scene.go( "..." )` names is in
the pack. True-3D games (plat3d) use scenes too: see docs/true-3d.md, "Scene changes in true 3D".

The test game: walk the hero (WASD) right into the cottage door in the meadow; inside, walk left out of the
doorway. The fairy follows both ways; the coin on the cottage floor stays gone once picked up.

## IDs (build-time string interning) and small maps

Names that scripts and data use at run time (animation tags, scenes, spawns, doors, songs, SoundFonts, sound
effects, colors, collision types, graph edges, genomes) get small integer ids at build time:
`tools/ids/genids.lua` scans every game's `genome/` tree, `main.lua` and `scripts/*.lua` and writes

* `gen/ids.h`: `enum class ids::Anim : uint8_t { DEFAULT = 0, crawl = 1, ..., COUNT }` (and `Scene`, `Spawn`, `Door`,
  `Song`, `Soundfont`, `Sound`, `Color`, `Coll`, `Edge`, `Genome`), with `ids::animNames[]` / `ids::animCount`;
* `gen/ids.lua`: `ids.anim.walk_up == 10`, `ids.names.anim[10] == "walk_up"` (`require("gen.ids")`).

`gen/` is generated (git-ignored). Nobody edits an id list: CMake runs the generator (on LuaJIT's host `minilua`,
built with LuaJIT, so nothing extra is installed on Linux, Windows CLANG64 or a cross build) whenever any of its
inputs changes, and the globs re-run CMake when files are added or removed. The Aseprite scripts (`sprite.lua`,
`adddoor.lua`, `addsong.lua`) regenerate it too right after they write (`scripts/aseprite/refreshids.lua`, using
Aseprite's own Lua), so new tags, scenes and spawns are known before the next build. By hand:

```sh
build/_deps/src/luajit/src/host/minilua tools/ids/genids.lua --root . --out gen --list build/ids-inputs.txt [--strict]
```

The generator also checks references: a door to a missing scene or spawn, a scene's song/SoundFont that isn't
in `genome/music/`, a genome's model or SoundFont that isn't there, a `self:sound("...")` that isn't one of that
genome's own sounds, a level placing a genome that doesn't exist, a literal `Scene.go("...")`,
`Sound.play("...")`/`Sound.key("...")` naming nothing, an
`initAnim` that isn't one of its sprite's tags, unknown collision types. Dev builds print these as warnings; release
builds (`BICYCLE_RELEASE`, `BICYCLE_RELEASE_ASSETS`) run it with `--strict`, the packs depend on it, and a typo fails
the build. Release embeds `gen/ids.lua` in the pack.

Scripts keep writing names: `anim:set( "walk_up" )`, `Scene.go( "house", { spawn = "entry" } )`,
`Sound.play( "coin" )`. `scripts/Ids.lua` resolves each once, at load (`Ids.get`; a name that's no tag/scene/spawn/sound
anywhere is an error that lists the known ones), and the engine works with ids from there: the Animator's strips are
a `bicycle::map` by tag id, doors resolve their target scene and spawn ids when they're armed, scenes are cached by
scene id with their spawns by spawn id, and sound effects are keyed by sound id. Names an asset brings that are
newer than `gen/ids.lua` (dev only) get the next free id with a "rebuild" warning.

**`bicycle::map<V, Pool>`** (`include/data/Map.h`) is the map for those ids: a port of jollybean's popcount-indexed
map (via mapbench's map2.c): a 256-bit presence bitfield plus prefix counts, values dense in key order in a
`mem` pool (template parameter or constructor, like `mem::Vector<T, POOL>`), lookups one bit test + one popcount,
ascending inserts append, overwrites in place, lazy x2 growth or exact capacity, non-trivial values allowed.
`-DBICYCLE_MAP_POPCNT_LUT` uses a byte table for CPUs without popcount. `ctest -R map`.

Per-entity data uses **`EntitySlots<T, Pool>`** (`include/m/EntitySlots.h`) instead of `unordered_map<Entity, T>`: a
flat pool-backed array by entity id whose slots remember the entity's generation (bumped by `delEntityId`), so a
recycled id never sees the previous owner's data (Scene's entity -> layer and hidden flags, the 3D renderer's models
and motion state).

## Memory pools

All engine memory comes from one arena cut into bump-allocated pools (`include/data/Mem.h`, after jollybean's
`mem.c`): PERMANENT (asset pack), GENERAL (every `new` in the engine, std containers), TEXT (the LuaJIT heap and
scripts), GRAPHIC (images, meshes, textures, framebuffers, GL staging), AUDIO (SoundFonts, MIDI) and TEMPORARY
(scratch, reset every frame and after each image load) and SCENE (what the current scene loaded; see Scene changes). `mem::Allocator` / `mem::Vector` / `mem::String` /
`mem::Resource` (pmr) put std containers in a pool. Sizes: `-DBICYCLE_MEM_<POOL>_MB=` or the same environment
variable; overflowing a pool is fatal and names it. `BICYCLE_MEM_STATS=1` prints high-water marks at exit.

Limits: LuaJIT takes the custom allocator only in GC64 builds (the default on x64/arm64, which the fetched LuaJIT
is); a non-GC64 build falls back to LuaJIT's own allocator with a warning. fluidsynth copies SoundFont samples into
its own malloc'd memory, so only the sf2/mid bytes are in AUDIO. Third-party C code (SDL, fluidsynth, GL drivers)
still uses malloc. fluidsynth's cpp11 thread code is patched at fetch time (new vs free mismatch).

## Input

Input is events, in every render mode: `onInput( self, key, pressed, dx, dy )` on the focused entity
(`focusOn( id )`) and on every listener (`listen = true` genomes, or `listenInput( id )`), in the order it happened.

* `key` is a `LogicalKey` (`include/c/Input.h`, `ffi.C.<name>`): `A`..`Z`, `Key0`..`Key9`, `Enter`, `Escape`,
  `Tab`, `Space`, `LeftShift`, `RightShift`, `LeftCtrl`, `RightCtrl`, `LeftAlt`, `RightAlt`, `LeftMeta`,
  `RightMeta`, `MouseLeft`, `MouseRight`, `MouseMiddle`, and `Look`. Arrows are `W`/`A`/`S`/`D`.
* `pressed` is `true` when the key or button goes down and `false` when it comes up: one press and one release per
  hold (auto-repeat is not a new press). Things that should keep happening while a key is held are a value set on
  the press and cleared on the release (`hero:move( right, fwd )`, `W.Camera.turn( yawRate, pitchRate )`) or a
  repeating timer started on the press and stopped on the release (the test hero's grid steps, fps3d's trigger).
* `Look` is mouse look: once per frame, only when the mouse moved (while captured, `Input3D.captureMouse`), with
  `dx, dy` the pixels since the last frame; the gamepad's right stick adds to it (600 px/s at full tilt). It is an
  event like the rest rather than a polled value or one event per motion report: one call a frame with the sum is
  the smallest form that keeps a frame's motion exact and costs nothing when the mouse is still.
* Gamepad (SDL): the left stick is `W`/`A`/`S`/`D` (past half tilt), A is `Space`, the right trigger and bumper are
  `MouseLeft`, the left stick's click is `LeftShift`.
* Dialogues and menus page and select on presses; releases don't reach them. During a scene transition presses are
  dropped but releases still arrive (a key held through a door is let go).
* `scripts/Input3D.lua` (held keys, axes, `Input.look()`) still works for code that polls.

**Terminals (ASCII mode, SSH).** Terminals send bytes, not key events, so `src/c/SshInputListener.cpp` with
`include/c/TermInput.h`:

* parses escape sequences (arrows are `W`/`A`/`S`/`D`, not `ESC [ A` = A; a lone ESC is `Escape` a frame later);
* asks for the kitty keyboard protocol (`CSI > 11 u`: kitty, foot, WezTerm, Ghostty, recent Alacritty and
  iTerm2...) and, if the terminal answers the `CSI ? u` query, uses its real press/repeat/release events;
* turns on SGR mouse (`?1000` + `?1006`): button presses and releases (`MouseLeft`/`MouseRight`/`MouseMiddle`);
* otherwise infers releases from auto-repeat: a key's first byte is a press, and while its repeat bytes keep coming
  it stays held; it's released when none arrives within the timeout: `initial` ms before the first repeat (the
  typematic delay; it adapts to the measured delay, within 0.5x..2x of `initial`), then `factor` times the measured
  repeat interval, clamped to [`min`, `max`]. Tune with `BICYCLE_TTY_KEYUP="initial,min,max,factor"` (default
  `500,60,150,2.5`). A key let go before its first repeat is released `initial` ms after it was pressed. xterm's
  modifyOtherKeys reports no releases, so xterm uses the heuristic. Terminals repeat only the newest key, so a key
  held under another one is released by its timeout. `BICYCLE_TTY_KITTY=0` / `BICYCLE_TTY_MOUSE=0` turn the
  protocols off; both are undone at exit.
* Tests: `tests/term_input_test.cpp` (ctest `term-input`: synthetic bytes and timings) and
  `tests/tty_input_test.py` (ctest `tty-input`, ASCII builds: the game in a pty).

`BICYCLE_INPUT_LOG=1` prints every event (`input <key> down|up`) to stderr, `=<file>` appends them to a file.

## Environment variables

* `BICYCLE_SCENE_STATS=1`: print the memory pools on every scene change. `BICYCLE_SCENE_CYCLE=<n>[:<a>:<b>:<frames>]`:
  make n scene changes between scenes a and b (default intro and house), one every <frames> frames, then quit
  (`BICYCLE_SCENE_CYCLE_TRANSITION=fade|wipe|dissolve|none` picks the transition).
* `BICYCLE_MODEL_ACTION=<action>`: 3D, pin every rigged model to one action.

`BICYCLE_MAX_FRAMES=<n>`: quit after n frames (headless tests).

`BICYCLE_SCRIPTED_INPUT="w@20-34/2,d@36-170"`: input on frame ranges: `/2` taps the key every 2nd frame
(pressed, released the next frame), no step (or `/1`) holds it from the first frame and releases it after the last.

`BICYCLE_LOOSE_ASSETS`, `BICYCLE_WINDOW_SIZE`, `BICYCLE_SCREENSHOT=<file.bmp>` (`%d` = frame number; in
ASCII the file is a text dump of the screen) (+ `BICYCLE_SCREENSHOT_FRAME`, default 90, or a list in
`BICYCLE_SCREENSHOT_FRAMES=40,41,42`, and `BICYCLE_EXIT_AFTER_SCREENSHOT=1`) and `BICYCLE_GL_INFO` for the pixel modes;

and `BICYCLE_AUDIO_DRIVER`, `BICYCLE_AUDIO_FILE`, `BICYCLE_GM_SF2`, `BICYCLE_MUTE_MUSIC`, `BICYCLE_PROFILE` (audio
start-up timings) and `BICYCLE_DEBUG_LOG` (Lua `dbglog`): see `include/c/MidiPlayer.h` and `tools/audio/README.md`.

## True 3D

The 3D build can run genuinely 3D games (third-person platformers, first-person shooters): 3D transforms and
character physics, a level collision mesh from Blender, raycasts, camera rigs driven by Kinematics, mouse look and
gamepads, shadows and fog. See [docs/true-3d.md](docs/true-3d.md); demos: `./o-3d plat3d`, `./o-3d fps3d`.
