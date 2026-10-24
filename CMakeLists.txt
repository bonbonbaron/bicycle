# bicycle: builds the engine executable `o` (o.exe on Windows) into the repository root, next to the games it
# runs (`./o test` runs test/main.lua). Needs only a C/C++ compiler, CMake and Ninja or Make:
#   cmake -B build && cmake --build build
# The first configure downloads and builds LuaJIT, fluidsynth and (on Windows) PDCurses, plus a General MIDI
# SoundFont; see cmake/BicycleDeps.cmake. -DBICYCLE_USE_SYSTEM_DEPS=ON uses installed packages instead.
cmake_minimum_required( VERSION 3.21 )
cmake_policy( SET CMP0091 NEW )  # MSVC runtime chosen by CMAKE_MSVC_RUNTIME_LIBRARY
set( CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" )  # same runtime as the fetched static libraries
project( bicycle C CXX )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
if ( NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES )
  # Optimized with debug info, the debug info split off into o.debug (BICYCLE_SPLIT_DEBUG below): a small, fast
  # executable that's still debuggable. It used to be Debug, which made o.exe ~16 MB, ~11 MB of it DWARF (a slower
  # first start: more to read from disk and for antivirus to scan). -DCMAKE_BUILD_TYPE=Debug for -O0 stepping.
  set( CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Debug, Release, RelWithDebInfo or MinSizeRel" FORCE )
endif()
option( BICYCLE_SPLIT_DEBUG "Move the executable's debug info into o.debug next to it (GCC/clang, not MSVC)" ON )
option( BICYCLE_LTO "Link-time optimization for the engine (smaller, slower to link)" OFF )
option( BICYCLE_SIZE "Optimize the engine for size (-Os)" OFF )

# How the game is drawn: ASCII (terminal, curses), 2D (SDL3 pixel graphics) or 3D (SDL3 + OpenGL 3.3).
set( BICYCLE_RENDER ASCII CACHE STRING "Render mode: ASCII, 2D or 3D" )
set_property( CACHE BICYCLE_RENDER PROPERTY STRINGS ASCII 2D 3D )
string( TOUPPER "${BICYCLE_RENDER}" BICYCLE_RENDER )
if ( NOT BICYCLE_RENDER MATCHES "^(ASCII|2D|3D)$" )
  message( FATAL_ERROR "BICYCLE_RENDER must be ASCII, 2D or 3D (got '${BICYCLE_RENDER}')." )
endif()
message( STATUS "bicycle: render mode ${BICYCLE_RENDER}" )

# Release: zero file I/O at run time (see "Release builds" below and README).
option( BICYCLE_GLES2 "3D: use OpenGL ES 2.0 (GLSL 100, no VAOs, no shadows) instead of trying GL 3.3 core first" OFF )
option( BICYCLE_RELEASE "Embed all assets and scripts in the executable; no file I/O at run time" OFF )
option( BICYCLE_EMBED_GM_SF2 "Release: embed sf2/default-GM.sf2 (the GM fallback, ~30 MB) in the executable" OFF )

include( cmake/BicycleDeps.cmake )
if ( BICYCLE_RENDER STREQUAL "ASCII" )
  bicycle_curses()
else()
  bicycle_sdl3()
endif()
bicycle_luajit()
# Build-time string interning (README, "IDs"): tools/ids/genids.lua scans every game's genome tree, main.lua and
# the engine scripts and writes gen/ids.h + gen/ids.lua (names -> small integer ids). It reruns whenever one of
# them changes (and the globs re-run CMake when files come or go), so nobody edits an id list by hand. Release
# builds run it with --strict: a door to a missing scene, a missing song or sound, a bad initAnim... fails the build.
if ( NOT BICYCLE_LUA_HOST )
  find_program( BICYCLE_LUA_HOST_PROGRAM NAMES luajit lua5.1 lua5.4 lua )
  if ( NOT BICYCLE_LUA_HOST_PROGRAM )
    message( FATAL_ERROR "bicycle: no Lua for tools/ids/genids.lua (LuaJIT's minilua wasn't built and no lua/luajit on PATH)" )
  endif()
  set( BICYCLE_LUA_HOST "${BICYCLE_LUA_HOST_PROGRAM}" )
endif()
file( GLOB repoDirs LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/*" )
set( idsInputs "${CMAKE_SOURCE_DIR}/include/v/common/ColorPalette.h" )
foreach( d ${repoDirs} )
  if ( IS_DIRECTORY "${d}/genome" )
    file( GLOB_RECURSE genomeFiles CONFIGURE_DEPENDS "${d}/genome/*" )
    list( FILTER genomeFiles EXCLUDE REGEX "/\\.gitkeep$|\\.bmesh$|\\.bskin$" )
    list( APPEND idsInputs ${genomeFiles} )
    if ( EXISTS "${d}/main.lua" )
      list( APPEND idsInputs "${d}/main.lua" )
    endif()
  endif()
endforeach()
file( GLOB idsScripts CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/*.lua" )
list( FILTER idsScripts EXCLUDE REGEX "/ids\\.lua$" )
list( APPEND idsInputs ${idsScripts} )
string( REPLACE ";" "\n" idsList "${idsInputs}" )
file( WRITE "${CMAKE_BINARY_DIR}/ids-inputs.txt.tmp" "${idsList}\n" )
file( COPY_FILE "${CMAKE_BINARY_DIR}/ids-inputs.txt.tmp" "${CMAKE_BINARY_DIR}/ids-inputs.txt" ONLY_IF_DIFFERENT )
set( idsStrict )
if ( BICYCLE_RELEASE OR BICYCLE_RELEASE_ASSETS )
  set( idsStrict --strict )
endif()
set( BICYCLE_IDS_STAMP "${CMAKE_BINARY_DIR}/ids.stamp" )
add_custom_command( OUTPUT "${BICYCLE_IDS_STAMP}"
  BYPRODUCTS "${CMAKE_SOURCE_DIR}/gen/ids.h" "${CMAKE_SOURCE_DIR}/gen/ids.lua"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_SOURCE_DIR}/gen"
  COMMAND "${BICYCLE_LUA_HOST}" "${CMAKE_SOURCE_DIR}/tools/ids/genids.lua" --root "${CMAKE_SOURCE_DIR}"
          --out "${CMAKE_SOURCE_DIR}/gen" --list "${CMAKE_BINARY_DIR}/ids-inputs.txt" --stamp "${BICYCLE_IDS_STAMP}" ${idsStrict}
  DEPENDS ${idsInputs} "${CMAKE_SOURCE_DIR}/tools/ids/genids.lua" "${CMAKE_BINARY_DIR}/ids-inputs.txt"
  COMMENT "Generating gen/ids.h and gen/ids.lua (string ids)" VERBATIM )
add_custom_target( ids ALL DEPENDS "${BICYCLE_IDS_STAMP}" )
# gen/ids.h exists before anything includes it (the first build runs the generator at configure time too).
if ( NOT EXISTS "${CMAKE_SOURCE_DIR}/gen/ids.h" OR NOT EXISTS "${CMAKE_SOURCE_DIR}/gen/ids.lua" )
  file( MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/gen" )
  execute_process( COMMAND "${BICYCLE_LUA_HOST}" "${CMAKE_SOURCE_DIR}/tools/ids/genids.lua" --root "${CMAKE_SOURCE_DIR}"
                   --out "${CMAKE_SOURCE_DIR}/gen" --list "${CMAKE_BINARY_DIR}/ids-inputs.txt" RESULT_VARIABLE idsRc )
  if ( NOT idsRc EQUAL 0 )
    message( FATAL_ERROR "bicycle: tools/ids/genids.lua failed" )
  endif()
endif()
bicycle_fluidsynth()
bicycle_gm_soundfont()
bicycle_miniz()
find_package( Threads REQUIRED )

# --- assets: codecs (inflate, strips, Edgebreaker meshes) and the packer -------------------------------------------
# The engine links decoders only; deflate and the encoders are offline (tools/codec, linked by bicycle-pack and
# bicycle-tests). See cmake/BicycleDataSources.cmake.
include( cmake/BicycleDataSources.cmake )
add_library( bicycle_data STATIC ${BICYCLE_DATA_SOURCES} )
target_include_directories( bicycle_data PUBLIC include )
# Memory pool sizes in MB (include/data/Mem.h); the same names as environment variables override them at run time.
foreach( pool_default PERMANENT=16 GENERAL=128 TEXT=64 GRAPHIC=96 AUDIO=96 TEMPORARY=64 SCENE=64 )
  string( REPLACE "=" ";" pd "${pool_default}" )
  list( GET pd 0 pool )
  list( GET pd 1 mb )
  set( BICYCLE_MEM_${pool}_MB ${mb} CACHE STRING "Size of the ${pool} memory pool in MB" )
  target_compile_definitions( bicycle_data PRIVATE BICYCLE_MEM_${pool}_MB=${BICYCLE_MEM_${pool}_MB} )
endforeach()

set( MINIZ_DIR "${BICYCLE_MINIZ_DIR}" )
if ( CMAKE_CROSSCOMPILING )
  # The packer runs during the build, so build it for this machine, not the target.
  # Its compilers are the host's (BICYCLE_HOST_CC/CXX), never the cross compilers in CC/CXX.
  set( BICYCLE_HOST_CC cc CACHE STRING "Host C compiler for build-time tools when cross-compiling" )
  set( BICYCLE_HOST_CXX c++ CACHE STRING "Host C++ compiler for build-time tools when cross-compiling" )
  set( hostBin "${BICYCLE_DEPS_DIR}/build/bicycle-pack-host" )
  set( hostExe "" )
  if ( CMAKE_HOST_WIN32 )
    set( hostExe ".exe" )
  endif()
  if ( NOT EXISTS "${hostBin}/CMakeCache.txt" )
    bicycle_run( "configuring the host asset packer" "${CMAKE_COMMAND}" -E env --unset=CC --unset=CXX --unset=CMAKE_TOOLCHAIN_FILE
      "${CMAKE_COMMAND}" -S "${CMAKE_SOURCE_DIR}/tools/pack" -B "${hostBin}" -G "${CMAKE_GENERATOR}" -DCMAKE_BUILD_TYPE=Release
      "-DCMAKE_C_COMPILER=${BICYCLE_HOST_CC}" "-DCMAKE_CXX_COMPILER=${BICYCLE_HOST_CXX}" "-DMINIZ_DIR=${MINIZ_DIR}" )
  endif()
  add_custom_target( bicycle-pack-host COMMAND "${CMAKE_COMMAND}" --build "${hostBin}" --config Release BYPRODUCTS "${hostBin}/bicycle-pack${hostExe}" )
  set( BICYCLE_PACKER "${hostBin}/bicycle-pack${hostExe}" )
  set( BICYCLE_PACKER_TARGET bicycle-pack-host )
else()
  add_subdirectory( tools/pack )
  set( BICYCLE_PACKER "$<TARGET_FILE:bicycle-pack>" )
  set( BICYCLE_PACKER_TARGET bicycle-pack )
endif()

# Dev (default): the engine reads loose, uncompressed assets straight from the game folder (Lua, .bimg, .obj
# meshes, .sf2/.mid): nothing to pack or compile after editing. Release (-DBICYCLE_RELEASE_ASSETS=ON): every
# build packs <game>/assets.bpak (compressed) and the engine reads assets from it. `cmake --build build --target
# package` packs in either mode, to finalize a game.
option( BICYCLE_RELEASE_ASSETS "Pack and compress assets every build and load them from the pack" OFF )
set( BICYCLE_GAMES "test" CACHE STRING "Game folders whose assets get packed" )
set( packs )
foreach( game ${BICYCLE_GAMES} )
  file( GLOB_RECURSE gameFiles CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/${game}/main.lua" "${CMAKE_SOURCE_DIR}/${game}/genome/*" )
  add_custom_command( OUTPUT "${CMAKE_SOURCE_DIR}/${game}/assets.bpak"
    COMMAND "${BICYCLE_PACKER}" "${CMAKE_SOURCE_DIR}/${game}" "${CMAKE_SOURCE_DIR}/${game}/assets.bpak"
    DEPENDS ${gameFiles} ${BICYCLE_PACKER_TARGET} "${BICYCLE_IDS_STAMP}"
    COMMENT "Packing ${game}'s assets" VERBATIM )
  list( APPEND packs "${CMAKE_SOURCE_DIR}/${game}/assets.bpak" )
endforeach()
# Release (-DBICYCLE_RELEASE=ON): zero file I/O at run time. The first game in BICYCLE_GAMES is packed together
# with the engine's Lua scripts (scripts/*.lua), UI fonts and, with BICYCLE_EMBED_GM_SF2, the General MIDI
# SoundFont, and that pack is linked into the executable (src/data/EmbeddedPack.c). The engine's file readers,
# Lua's file searchers/io.open/dofile and fluidsynth's file loader are compiled out or disabled; see README.
if ( BICYCLE_RELEASE )
  # A plain variable, not the cache: forcing the cache option ON here used to leave it ON after BICYCLE_RELEASE was
  # turned off again in the same build directory, so that "dev" build kept mounting a (stale) <game>/assets.bpak.
  set( BICYCLE_RELEASE_ASSETS ON )
  if ( MSVC )
    message( FATAL_ERROR "BICYCLE_RELEASE embeds the pack with .incbin: build with GCC/clang/MinGW/llvm-mingw." )
  endif()
  list( GET BICYCLE_GAMES 0 BICYCLE_RELEASE_GAME )
  set( game "${BICYCLE_RELEASE_GAME}" )
  file( GLOB_RECURSE gameFiles CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/${game}/main.lua" "${CMAKE_SOURCE_DIR}/${game}/genome/*" )
  file( GLOB engineScripts CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/*.lua" )
  # A game's own fonts/ replace the engine's (PixelRenderer looks in <game>/fonts first).
  file( GLOB fontFiles CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/${game}/fonts/*.ttf" )
  if ( NOT fontFiles )
    file( GLOB fontFiles CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/fonts/*.ttf" )
  endif()
  set( addArgs )
  foreach( f ${engineScripts} ${fontFiles} )
    get_filename_component( dir "${f}" DIRECTORY )
    get_filename_component( dir "${dir}" NAME )
    get_filename_component( name "${f}" NAME )
    list( APPEND addArgs --add "${f}" "${dir}/${name}" )
  endforeach()
  # The generated ids (require("gen.ids")): the generator ran --strict first (its stamp is a dependency).
  list( APPEND addArgs --add "${CMAKE_SOURCE_DIR}/gen/ids.lua" "gen/ids.lua" )
  set( extraFiles ${engineScripts} ${fontFiles} "${BICYCLE_IDS_STAMP}" )
  if ( BICYCLE_EMBED_GM_SF2 )
    if ( NOT EXISTS "${CMAKE_SOURCE_DIR}/sf2/default-GM.sf2" )
      message( FATAL_ERROR "BICYCLE_EMBED_GM_SF2: no sf2/default-GM.sf2" )
    endif()
    list( APPEND addArgs --add "${CMAKE_SOURCE_DIR}/sf2/default-GM.sf2" "sf2/default-GM.sf2" )
    list( APPEND extraFiles "${CMAKE_SOURCE_DIR}/sf2/default-GM.sf2" )
  endif()
  set( BICYCLE_EMBEDDED_PACK "${CMAKE_BINARY_DIR}/embedded-${game}.bpak" )
  add_custom_command( OUTPUT "${BICYCLE_EMBEDDED_PACK}"
    COMMAND "${BICYCLE_PACKER}" "${CMAKE_SOURCE_DIR}/${game}" "${BICYCLE_EMBEDDED_PACK}" ${addArgs}
    DEPENDS ${gameFiles} ${extraFiles} ${BICYCLE_PACKER_TARGET}
    COMMENT "Packing ${game}, the engine scripts and fonts for embedding" VERBATIM )
  add_custom_target( embedded-pack DEPENDS "${BICYCLE_EMBEDDED_PACK}" )
endif()
if ( BICYCLE_RELEASE_ASSETS )
  add_custom_target( assets ALL DEPENDS ${packs} )
else()
  add_custom_target( assets DEPENDS ${packs} )
endif()
add_custom_target( package DEPENDS ${packs} )
# A private pack of the test game for the `codecs` ctest (always repacked, so it matches the sources).
add_custom_target( codecs-pack
  COMMAND "${BICYCLE_PACKER}" "${CMAKE_SOURCE_DIR}/test" "${CMAKE_BINARY_DIR}/codecs-test.bpak"
  DEPENDS ${BICYCLE_PACKER_TARGET} ids VERBATIM )

# Headless codec tests: `ctest --test-dir build` (or build/bicycle-tests test).
if ( NOT CMAKE_CROSSCOMPILING )
  add_executable( bicycle-motion-tests tests/motion_test.cpp )
  target_include_directories( bicycle-motion-tests PRIVATE include )
  add_executable( bicycle-tests tests/codec_test.cpp )
  target_link_libraries( bicycle-tests PRIVATE bicycle_codecs_encode )
  enable_testing()
  # The pack round-trip packs the test game afresh into this build directory and checks that pack. Not
  # test/assets.bpak: it's shared by every build directory and only rebuilt by builds whose BICYCLE_GAMES has
  # "test", so in a plat3d/fps3d build it was stale after any test/genome edit (the flaky `codecs` failure).
  set( codecsPack "${CMAKE_BINARY_DIR}/codecs-test.bpak" )
  add_test( NAME pack-assets COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target package codecs-pack )
  set_tests_properties( pack-assets PROPERTIES FIXTURES_SETUP packed )
  add_test( NAME codecs COMMAND bicycle-tests "test=${codecsPack}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" )
  add_test( NAME motion COMMAND bicycle-motion-tests )
  # Terminal input (include/c/TermInput.h): escape sequences, kitty/SGR events, the key-release heuristic.
  add_executable( bicycle-term-input-tests tests/term_input_test.cpp )
  target_include_directories( bicycle-term-input-tests PRIVATE include )
  add_test( NAME term-input COMMAND bicycle-term-input-tests )
  add_executable( bicycle-mem-scene-tests tests/mem_scene_test.cpp src/data/Mem.cpp )
  target_include_directories( bicycle-mem-scene-tests PRIVATE include )
  add_test( NAME mem-scene COMMAND bicycle-mem-scene-tests )
  # bicycle::map (include/data/Map.h), with a byte-table popcount build too (BICYCLE_MAP_POPCNT_LUT).
  add_executable( bicycle-map-tests tests/map_test.cpp src/data/Mem.cpp )
  target_include_directories( bicycle-map-tests PRIVATE include )
  add_test( NAME map COMMAND bicycle-map-tests )
  add_test( NAME ids COMMAND sh "${CMAKE_SOURCE_DIR}/tests/ids_test.sh" "${BICYCLE_LUA_HOST}" "${CMAKE_SOURCE_DIR}" )
  add_executable( bicycle-map-lut-tests tests/map_test.cpp src/data/Mem.cpp )
  target_include_directories( bicycle-map-lut-tests PRIVATE include )
  target_compile_definitions( bicycle-map-lut-tests PRIVATE BICYCLE_MAP_POPCNT_LUT )
  add_test( NAME map-lut COMMAND bicycle-map-lut-tests )
  # ActiveFirst (include/data/ActiveFirst.h), the active-first collision check and its step blocking, Kinematics'
  # and Timer's active sets (Trigger stubbed in the test). bicycle-collision-bench: the old all-pairs check vs this.
  set( activeFirstSources src/c/CollisionDetector.cpp src/c/Kinematics.cpp src/c/Timer.cpp src/c/Layer3DOff.cpp
       src/c/ImageOverlaps.cpp src/m/World.cpp src/m/Box.cpp src/v/common/Image.cpp src/v/common/ColorPalette.cpp src/data/Mem.cpp )
  add_executable( bicycle-active-first-tests tests/active_first_test.cpp ${activeFirstSources} )
  add_executable( bicycle-collision-bench tests/collision_bench.cpp ${activeFirstSources} )
  foreach( t bicycle-active-first-tests bicycle-collision-bench )
    target_include_directories( ${t} PRIVATE include "${CMAKE_SOURCE_DIR}" )
    target_link_libraries( ${t} PRIVATE bicycle_luajit )  # Trigger.h includes lua.hpp
    add_dependencies( ${t} ids )
  endforeach()
  add_test( NAME active-first COMMAND bicycle-active-first-tests )
  # True 3D: math (include/m/Math3D.h), level collision, character physics and raycasts (src/c/Physics3D.cpp).
  add_executable( bicycle-3d-tests tests/math3d_test.cpp src/c/Physics3D.cpp )
  target_link_libraries( bicycle-3d-tests PRIVATE bicycle_data )
  if ( BICYCLE_RELEASE )  # release bicycle_data reads only the embedded pack
    target_sources( bicycle-3d-tests PRIVATE src/data/EmbeddedPack.c )
    add_dependencies( bicycle-3d-tests embedded-pack )
  endif()
  add_test( NAME math3d COMMAND bicycle-3d-tests )
  set_tests_properties( codecs PROPERTIES FIXTURES_REQUIRED packed )
endif()

set( BICYCLE_SOURCES
  src/main.cpp src/bicycle.cpp src/trigLUT.cpp src/data/MemNew.cpp
  src/lua/LuaWindows.cpp src/lua/LuaScene.cpp src/lua/LuaGraphics.cpp src/lua/LuaMotion.cpp src/lua/LuaAudio.cpp
  src/lua/LuaHud.cpp src/lua/LuaSystem.cpp
  src/c/InputListener.cpp src/c/ScriptedInput.cpp src/v/common/PixelImages.cpp
  src/m/Entity.cpp src/m/Box.cpp src/m/Camera.cpp src/m/World.cpp
  src/v/common/Image.cpp src/v/common/ColorPalette.cpp src/v/common/Window.cpp src/v/common/Dialogue.cpp
  src/v/common/Menu.cpp src/v/common/TextMenu.cpp src/v/common/Scene.cpp
  src/c/Trigger.cpp src/c/Timer.cpp src/c/WindowManager.cpp src/c/CollisionDetector.cpp
  src/c/Kinematics.cpp src/c/Animator.cpp src/c/MidiPlayer.cpp src/c/Dice.cpp
  src/c/InputState.cpp )
# Each mode adds only its own code: the true-3D layer (physics, 3D motions and camera rig, their Lua API) is in 3D
# builds alone; ASCII and 2D link its no-op stand-in, src/c/Layer3DOff.cpp (include/c/Layer3D.h). The flat modes'
# image overlap groups (depth sorting, include/c/ImageOverlaps.h) are in ASCII and 2D alone; 3D links ImageOverlapsOff.
# Not built (unfinished or replaced): src/c/RenderSystem.cpp, src/v/common/Graph.cpp
if ( BICYCLE_RENDER STREQUAL "ASCII" )
	#list( APPEND BICYCLE_SOURCES src/v/ascii/AsciiRenderer.cpp src/c/SshInputListener.cpp src/c/Layer3DOff.cpp src/c/ImageOverlaps.cpp )
  # InputListener's terminal backends (include/c/InputListener.h): SshInput reads stdin; evdev on Linux.
  list( APPEND BICYCLE_SOURCES src/v/ascii/AsciiRenderer.cpp src/c/SshInputListener.cpp src/c/Layer3DOff.cpp src/c/ImageOverlaps.cpp )
elseif ( BICYCLE_RENDER STREQUAL "2D" )
  list( APPEND BICYCLE_SOURCES src/v/sdl/PixelRenderer.cpp src/v/sdl/UiFont.cpp src/v/sdl/SpriteAtlas.cpp src/v/2d/Renderer2D.cpp src/c/Layer3DOff.cpp
    src/c/ImageOverlaps.cpp )
elseif ( BICYCLE_RENDER STREQUAL "3D" )
  list( APPEND BICYCLE_SOURCES src/v/sdl/PixelRenderer.cpp src/v/sdl/UiFont.cpp src/v/sdl/SpriteAtlas.cpp src/v/3d/Renderer3D.cpp
    src/c/Layer3D.cpp src/c/Physics3D.cpp src/c/World3DApi.cpp src/c/Kinematics3D.cpp src/c/ImageOverlapsOff.cpp )
endif()

if ( BICYCLE_RELEASE )
  list( APPEND BICYCLE_SOURCES src/data/EmbeddedPack.c )
  set_source_files_properties( src/data/EmbeddedPack.c PROPERTIES
    COMPILE_DEFINITIONS "BICYCLE_EMBEDDED_PACK=\"${BICYCLE_EMBEDDED_PACK}\""
    OBJECT_DEPENDS "${BICYCLE_EMBEDDED_PACK}" )
endif()
add_executable( o ${BICYCLE_SOURCES} )
add_dependencies( o ids )
target_include_directories( o PRIVATE "${CMAKE_SOURCE_DIR}" )  # gen/ids.h
target_include_directories( o PRIVATE include )
if ( BICYCLE_GLES2 )
  target_compile_definitions( o PRIVATE BICYCLE_GLES2 )
endif()
# 32-bit ARM cross builds with clang/zig (compiler-rt): libgcc-named helpers LuaJIT calls.
if ( CMAKE_SYSTEM_PROCESSOR MATCHES "^arm" AND CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_SIZEOF_VOID_P EQUAL 4 )
  target_sources( o PRIVATE src/platform/arm32_libgcc_compat.c )
endif()
target_link_libraries( o PRIVATE bicycle_data bicycle_luajit bicycle_fluidsynth Threads::Threads )
target_compile_definitions( o PRIVATE BICYCLE_RENDER_${BICYCLE_RENDER} )
if ( BICYCLE_RELEASE_ASSETS )
  target_compile_definitions( o PRIVATE BICYCLE_RELEASE_ASSETS )
  target_compile_definitions( bicycle_data PRIVATE BICYCLE_RELEASE_ASSETS )  # Pack.cpp: dev builds never mount a pack
endif()
if ( BICYCLE_RELEASE )
  target_compile_definitions( o PRIVATE BICYCLE_RELEASE "BICYCLE_RELEASE_GAME=\"${BICYCLE_RELEASE_GAME}\"" )
  target_compile_definitions( bicycle_data PRIVATE BICYCLE_RELEASE )
  add_dependencies( o embedded-pack )
endif()
if ( BICYCLE_RENDER STREQUAL "ASCII" )
  target_link_libraries( o PRIVATE bicycle_curses )
  if ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )  # InputListener reads /dev/input keyboards through libevdev
    find_package( PkgConfig REQUIRED )
    pkg_check_modules( LIBEVDEV REQUIRED IMPORTED_TARGET libevdev )
    target_link_libraries( o PRIVATE PkgConfig::LIBEVDEV )
  endif()
else()
  target_link_libraries( o PRIVATE bicycle_sdl3 )
endif()
# The executable is o (ASCII), o-2d or o-3d, so builds of different modes can sit side by side in the repo root.
# Where o/o-2d/o-3d go: the repo root (default), or e.g. the build directory for cross builds, so they don't
# replace the machine's own executables.
set( BICYCLE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}" CACHE PATH "Directory for the o/o-2d/o-3d executable" )
set( BICYCLE_EXE_NAME o )
if ( NOT BICYCLE_RENDER STREQUAL "ASCII" )
  string( TOLOWER "o-${BICYCLE_RENDER}" BICYCLE_EXE_NAME )
endif()
set_target_properties( o PROPERTIES
  OUTPUT_NAME ${BICYCLE_EXE_NAME}
  RUNTIME_OUTPUT_DIRECTORY ${BICYCLE_OUTPUT_DIR}
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BICYCLE_OUTPUT_DIR}
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BICYCLE_OUTPUT_DIR}
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BICYCLE_OUTPUT_DIR}
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${BICYCLE_OUTPUT_DIR}
  # Lua finds the engine's extern "C" functions through LuaJIT's FFI (ffi.C.*), which looks them up among the
  # executable's exported symbols: -rdynamic on Linux, an export table on Windows.
  ENABLE_EXPORTS ON
  WINDOWS_EXPORT_ALL_SYMBOLS ON )
if ( MSVC )
  target_compile_options( o PRIVATE /W3 /utf-8 /Zc:__cplusplus )
  target_compile_definitions( o PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX )
else()
  target_compile_options( o PRIVATE -Wall -Wno-switch )
endif()
if ( NOT MSVC )
  # Unused code and data dropped at link time; exported (FFI) functions are roots, so they stay.
  target_compile_options( o PRIVATE -ffunction-sections -fdata-sections $<$<BOOL:${BICYCLE_SIZE}>:-Os> )
  # Debug: DWARF 4, which every gdb reads (clang targeting MinGW emits DWARF, not CodeView, with -g).
  target_compile_options( o PRIVATE $<$<CONFIG:Debug>:-gdwarf-4> )
  if ( APPLE )
    target_link_options( o PRIVATE -Wl,-dead_strip )
  else()
    target_link_options( o PRIVATE -Wl,--gc-sections )
  endif()
  # Release (zero-I/O) builds ship: no symbols or debug info at all.
  if ( BICYCLE_RELEASE AND NOT APPLE )
    target_link_options( o PRIVATE -s )
  endif()
endif()
# The systems' per-frame loops over their active rows (ActiveFirst) are written to vectorize; GCC's -O2 cost model
# ("very cheap") refuses any loop of unknown length, so these files use the "cheap" one (clang vectorizes at -O2).
set_source_files_properties( src/c/CollisionDetector.cpp src/c/Kinematics.cpp src/c/Timer.cpp src/c/ImageOverlaps.cpp
  PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:GNU>:-fvect-cost-model=cheap>" )
if ( BICYCLE_LTO )
  set_property( TARGET o PROPERTY INTERPROCEDURAL_OPTIMIZATION ON )
endif()
# Debug info out of the executable into o.debug (o-2d.debug, ...), linked back with a .gnu_debuglink, which gdb
# and lldb follow. Builds without -g skip it.
# Only RelWithDebInfo: Debug keeps its DWARF in the executable, and Windows never splits (gdb there doesn't reliably
# follow a PE's .gnu_debuglink, which leaves every breakpoint unbound).
if ( BICYCLE_SPLIT_DEBUG AND NOT MSVC AND NOT APPLE AND NOT WIN32 AND NOT BICYCLE_RELEASE
     AND CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" )
  find_program( BICYCLE_OBJCOPY NAMES llvm-objcopy ${CMAKE_OBJCOPY} objcopy )
  if ( BICYCLE_OBJCOPY )
    add_custom_command( TARGET o POST_BUILD
      COMMAND "${BICYCLE_OBJCOPY}" --only-keep-debug "$<TARGET_FILE:o>" "$<TARGET_FILE:o>.debug"
      COMMAND "${BICYCLE_OBJCOPY}" --strip-debug "--add-gnu-debuglink=$<TARGET_FILE:o>.debug" "$<TARGET_FILE:o>"
      WORKING_DIRECTORY "$<TARGET_FILE_DIR:o>" VERBATIM )
  endif()
endif()
if ( MINGW )
  # Export the engine functions for the FFI, and link the compiler's runtime (libstdc++/libc++, winpthread)
  # statically so o.exe runs without MinGW's DLLs on PATH.
  target_link_options( o PRIVATE -Wl,--export-all-symbols -static )
endif()

if ( WIN32 )
  # Fetched dependencies are static. Installed ones (BICYCLE_USE_SYSTEM_DEPS) may be DLLs: copy them next to o.exe.
  add_custom_command( TARGET o POST_BUILD
    COMMAND ${CMAKE_COMMAND} "-DDLLS=$<TARGET_RUNTIME_DLLS:o>" "-DDEST=$<TARGET_FILE_DIR:o>"
            -P ${CMAKE_SOURCE_DIR}/cmake/CopyDlls.cmake
    VERBATIM )
endif()

# ctest runs this build directory's own copy of the executable. The one in the repo root is shared by every build
# directory of the same mode (a plat3d and an fps3d release build both write o-3d), so tests of one build could run
# another's executable (`This release build runs only its embedded game`).
set( BICYCLE_TEST_EXE "${CMAKE_BINARY_DIR}/test-bin/${BICYCLE_EXE_NAME}${CMAKE_EXECUTABLE_SUFFIX}" )
add_custom_command( TARGET o POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/test-bin"
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:o>" "${BICYCLE_TEST_EXE}"
  VERBATIM )

# Release builds: run the game headless for 120 frames under strace and fail if it opens anything but OS files
# (tests/release_io_test.sh lists what's allowed and why). Needs strace; 2D/3D also xvfb-run (or a DISPLAY).
if ( BICYCLE_RELEASE AND NOT CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  string( TOLOWER "${BICYCLE_RENDER}" modeLower )
  add_test( NAME release-zero-io COMMAND sh "${CMAKE_SOURCE_DIR}/tests/release_io_test.sh" "${BICYCLE_TEST_EXE}" ${modeLower} 120 )
  set_tests_properties( release-zero-io PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300 )
  # Games with scenes change scenes during the run too (loads after start-up must come from the pack as well).
  if ( BICYCLE_RELEASE_GAME STREQUAL "test" )
    set_tests_properties( release-zero-io PROPERTIES ENVIRONMENT "BICYCLE_SCENE_CYCLE=2:intro:house:20" )
  elseif ( BICYCLE_RELEASE_GAME STREQUAL "plat3d" AND BICYCLE_RENDER STREQUAL "3D" )
    set_tests_properties( release-zero-io PROPERTIES ENVIRONMENT "BICYCLE_SCENE_CYCLE=2:courtyard:bonus:20" )
  endif()
endif()

# Scene changes: back and forth between the test game's two scenes; the SCENE pool must empty on every unload and
# its high-water mark stay flat (tests/scene_lifecycle_test.sh). Dev and release builds alike.
if ( NOT CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  string( TOLOWER "${BICYCLE_RENDER}" modeLowerLc )
  # Which two scenes to walk between, per game (the first of BICYCLE_GAMES; games without scenes skip the test).
  list( GET BICYCLE_GAMES 0 lifecycleGame )
  set( lifecyclePair "" )
  if ( lifecycleGame STREQUAL "test" )
    set( lifecyclePair "intro:house" )
  elseif ( lifecycleGame STREQUAL "plat3d" AND BICYCLE_RENDER STREQUAL "3D" )
    set( lifecyclePair "courtyard:bonus" )
  endif()
  # ffi.C.setVelX/Y/Z from a genome (tests/velx_test.sh): declared in the cdef, and the entity actually moves.
  if ( BICYCLE_RENDER STREQUAL "ASCII" AND NOT BICYCLE_RELEASE )
    add_test( NAME set-vel-x COMMAND sh "${CMAKE_SOURCE_DIR}/tests/velx_test.sh" "${BICYCLE_TEST_EXE}" "${CMAKE_SOURCE_DIR}" )
    set_tests_properties( set-vel-x PROPERTIES TIMEOUT 200 )
  endif()
  # plat3d's coins (tests/coin_collect_test.sh): removing a body inside its collision dispatch. 3D dev builds run
  # loose files; a release build needs plat3d packed.
  if ( BICYCLE_RENDER STREQUAL "3D" AND ( NOT BICYCLE_RELEASE OR lifecycleGame STREQUAL "plat3d" ) )
    add_test( NAME coin-collect COMMAND sh "${CMAKE_SOURCE_DIR}/tests/coin_collect_test.sh" "${BICYCLE_TEST_EXE}"
              WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" )
    set_tests_properties( coin-collect PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 400 )
  endif()
  if ( lifecyclePair )
    add_test( NAME scene-lifecycle COMMAND sh "${CMAKE_SOURCE_DIR}/tests/scene_lifecycle_test.sh" "${BICYCLE_TEST_EXE}" ${modeLowerLc} 20
              ${lifecycleGame} ${lifecyclePair} WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" )
    set_tests_properties( scene-lifecycle PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 400 )
  endif()
  # Terminal key events end to end (tests/tty_input_test.py): auto-repeat bytes, an arrow, SGR mouse and kitty
  # keyboard events into the ASCII build through a pty, each hold one press and one release in Lua's onInput.
  if ( BICYCLE_RENDER STREQUAL "ASCII" AND lifecycleGame STREQUAL "test" )
    find_package( Python3 COMPONENTS Interpreter QUIET )
    if ( Python3_FOUND )
      add_test( NAME tty-input COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/tty_input_test.py" "${BICYCLE_TEST_EXE}"
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" )
      set_tests_properties( tty-input PROPERTIES TIMEOUT 120 )
    endif()
  endif()
  # The Aseprite door/song scripts in batch mode (skipped unless BICYCLE_ASEPRITE points at an aseprite binary).
  set( BICYCLE_ASEPRITE "$ENV{BICYCLE_ASEPRITE}" CACHE FILEPATH "Aseprite binary for the aseprite-scene-scripts test" )
  add_test( NAME aseprite-scene-scripts COMMAND sh "${CMAKE_SOURCE_DIR}/tests/aseprite_scene_scripts_test.sh" "${CMAKE_SOURCE_DIR}" )
  set_tests_properties( aseprite-scene-scripts PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT "BICYCLE_ASEPRITE=${BICYCLE_ASEPRITE}" )
  # The Blender add-on (tools/blender) in background mode (skipped unless BICYCLE_BLENDER points at blender).
  set( BICYCLE_BLENDER "$ENV{BICYCLE_BLENDER}" CACHE FILEPATH "Blender binary for the blender-addon test" )
  add_test( NAME blender-addon COMMAND sh "${CMAKE_SOURCE_DIR}/tests/blender_addon_test.sh" "${CMAKE_SOURCE_DIR}" )
  set_tests_properties( blender-addon PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT "BICYCLE_BLENDER=${BICYCLE_BLENDER}" )
endif()
