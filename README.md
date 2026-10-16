# bicycle

A terminal game engine: ncurses (PDCurses on Windows) graphics, LuaJIT game scripts, fluidsynth music and
sound effects. The engine is one executable, `o`, that runs a game folder: `./o test` runs `test/main.lua`.

## Building on Linux

Packages (Debian/Ubuntu): `cmake g++ pkg-config libncurses-dev libluajit-5.1-dev libfluidsynth-dev`, plus a
General MIDI SoundFont for the music fallback: `fluid-soundfont-gm` (or `timgm6mb-soundfont`).

```sh
cmake --preset linux && cmake --build --preset linux    # or just: make
./o test
```

`make` is a thin wrapper around the same CMake build (`make clean`, `make install` too). The executable lands in
the repository root because games and `scripts/` are found relative to the current folder.

## Building on Windows

### Visual Studio (MSVC) + vcpkg

1. Install Visual Studio 2022 with "Desktop development with C++" (it includes CMake), and
   [vcpkg](https://github.com/microsoft/vcpkg):
   ```bat
   git clone https://github.com/microsoft/vcpkg C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   setx VCPKG_ROOT C:\vcpkg
   ```
2. In a new "x64 Native Tools Command Prompt for VS 2022", from the repository root:
   ```bat
   cmake --preset windows-msvc
   cmake --build --preset windows-msvc
   ```
   vcpkg reads `vcpkg.json` and builds PDCurses, LuaJIT and fluidsynth (and their dependencies, e.g. glib) the
   first time, which takes a while. It copies their DLLs next to `o.exe`.
3. Run a game from the repository root: `o test` (or `o.bat test`, or `.\o.ps1 test` in PowerShell).

Opening the folder in Visual Studio ("Open a local folder") also works: pick the `windows-msvc` preset.

### MSYS2 / MinGW-w64

In an MSYS2 UCRT64 shell:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,pkgconf,pdcurses,luajit,fluidsynth}
cmake --preset windows-mingw && cmake --build --preset windows-mingw
./o.exe test
```

Run it from the UCRT64 shell (or put `C:\msys64\ucrt64\bin` on `PATH`) so it finds the DLLs.

### Music on Windows

The General MIDI fallback SoundFont is looked up at `BICYCLE_GM_SF2`, then `./sf2/default-GM.sf2` (the
Linux system paths don't exist on Windows). Put any GM SoundFont there, e.g. FluidR3_GM.sf2 or TimGM6mb.sf2.
Without one, games still play with their own SoundFonts. See `tools/audio/README.md`.

### How Windows differs

* Keyboard: the console's `_kbhit()`/`_getch()` instead of a raw (termios) terminal.
* Terminal resizes: `GetConsoleScreenBufferInfo()` instead of `ioctl(TIOCGWINSZ)`.
* Curses: PDCurses (wincon port) instead of ncurses, both through `include/curses_compat.h`.
* Lua calls the engine with LuaJIT's FFI (`ffi.C.*`), which finds functions among the executable's exports.
  CMake exports them (`ENABLE_EXPORTS` and `WINDOWS_EXPORT_ALL_SYMBOLS`; `--export-all-symbols` on MinGW).
  The FFI declarations in `scripts/bicycle.lua` only use fixed-size types, so they're the same on both.
* `scripts/Scene.lua` lists folders with `dir /b` instead of `ls`.

### Cross-compiling o.exe from Linux (a check, not a release build)

`cmake/mingw-w64.cmake` is a MinGW-w64 toolchain file. With Windows builds of PDCurses, LuaJIT (both build
easily with MinGW) and fluidsynth (its release zip) under `~/win64`:

```sh
cmake -S . -B build/mingw-cross -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake -DCMAKE_PREFIX_PATH=$HOME/win64 \
      -DFLUIDSYNTH_LIBRARY=$HOME/win64/bin/libfluidsynth-3.dll -DFLUIDSYNTH_INCLUDE_DIR=$HOME/win64/include
cmake --build build/mingw-cross
```

The resulting `o.exe` runs under Wine with those DLLs (plus fluidsynth's SDL3.dll) next to it.

## Environment variables

`BICYCLE_AUDIO_DRIVER`, `BICYCLE_AUDIO_FILE`, `BICYCLE_GM_SF2`, `BICYCLE_MUTE_MUSIC`, `BICYCLE_PROFILE` (audio
start-up timings) and `BICYCLE_DEBUG_LOG` (Lua `dbglog`): see `include/c/MidiPlayer.h` and `tools/audio/README.md`.
