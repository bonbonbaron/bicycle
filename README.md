# bicycle

A terminal game engine: ncurses (PDCurses on Windows) graphics, LuaJIT game scripts, fluidsynth music and
sound effects. The engine is one executable, `o`, that runs a game folder: `./o test` runs `test/main.lua`.

## Building

Everything the engine needs (LuaJIT, fluidsynth, PDCurses on Windows, and a General MIDI SoundFont) is
downloaded from pinned URLs (checked against SHA-256 hashes) and built from source the first time you configure.
You only need CMake (3.21+), a C/C++ compiler and an internet connection; git isn't needed for the dependencies.

```sh
cmake -B build && cmake --build build
./o test            # o.exe test on Windows
```

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

The resulting `o.exe` runs under Wine.

## Environment variables

`BICYCLE_AUDIO_DRIVER`, `BICYCLE_AUDIO_FILE`, `BICYCLE_GM_SF2`, `BICYCLE_MUTE_MUSIC`, `BICYCLE_PROFILE` (audio
start-up timings) and `BICYCLE_DEBUG_LOG` (Lua `dbglog`): see `include/c/MidiPlayer.h` and `tools/audio/README.md`.
