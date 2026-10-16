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
  set( CMAKE_BUILD_TYPE Debug )  # same as the old Makefile's -g
endif()

# How the game is drawn: ASCII (terminal, curses), 2D (SDL3 pixel graphics) or 3D (SDL3 + OpenGL 3.3).
set( BICYCLE_RENDER ASCII CACHE STRING "Render mode: ASCII, 2D or 3D" )
set_property( CACHE BICYCLE_RENDER PROPERTY STRINGS ASCII 2D 3D )
string( TOUPPER "${BICYCLE_RENDER}" BICYCLE_RENDER )
if ( NOT BICYCLE_RENDER MATCHES "^(ASCII|2D|3D)$" )
  message( FATAL_ERROR "BICYCLE_RENDER must be ASCII, 2D or 3D (got '${BICYCLE_RENDER}')." )
endif()
message( STATUS "bicycle: render mode ${BICYCLE_RENDER}" )

# Release: zero file I/O at run time (see "Release builds" below and README).
option( BICYCLE_RELEASE "Embed all assets and scripts in the executable; no file I/O at run time" OFF )
option( BICYCLE_EMBED_GM_SF2 "Release: embed sf2/default-GM.sf2 (the GM fallback, ~30 MB) in the executable" OFF )

include( cmake/BicycleDeps.cmake )
if ( BICYCLE_RENDER STREQUAL "ASCII" )
  bicycle_curses()
else()
  bicycle_sdl3()
endif()
bicycle_luajit()
bicycle_fluidsynth()
bicycle_gm_soundfont()
bicycle_miniz()
find_package( Threads REQUIRED )

# --- assets: codecs (inflate, strips, Edgebreaker meshes) and the packer -------------------------------------------
add_library( bicycle_data STATIC
  src/data/tinfl.c src/data/Mem.cpp src/data/Pack.cpp src/data/Obj.cpp src/data/Atlas.cpp src/data/Skin.cpp src/data/Strip.cpp src/data/BImg.cpp src/data/Mesh.cpp
  src/data/Edgebreaker.cpp )
target_include_directories( bicycle_data PUBLIC include )
# Memory pool sizes in MB (include/data/Mem.h); the same names as environment variables override them at run time.
foreach( pool_default PERMANENT=16 GENERAL=128 TEXT=64 GRAPHIC=96 AUDIO=96 TEMPORARY=64 )
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
    DEPENDS ${gameFiles} ${BICYCLE_PACKER_TARGET}
    COMMENT "Packing ${game}'s assets" VERBATIM )
  list( APPEND packs "${CMAKE_SOURCE_DIR}/${game}/assets.bpak" )
endforeach()
# Release (-DBICYCLE_RELEASE=ON): zero file I/O at run time. The first game in BICYCLE_GAMES is packed together
# with the engine's Lua scripts (scripts/*.lua), UI fonts and, with BICYCLE_EMBED_GM_SF2, the General MIDI
# SoundFont, and that pack is linked into the executable (src/data/EmbeddedPack.c). The engine's file readers,
# Lua's file searchers/io.open/dofile and fluidsynth's file loader are compiled out or disabled; see README.
if ( BICYCLE_RELEASE )
  set( BICYCLE_RELEASE_ASSETS ON CACHE BOOL "" FORCE )
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
  set( extraFiles ${engineScripts} ${fontFiles} )
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

# Headless codec tests: `ctest --test-dir build` (or build/bicycle-tests test).
if ( NOT CMAKE_CROSSCOMPILING )
  add_executable( bicycle-motion-tests tests/motion_test.cpp )
  target_include_directories( bicycle-motion-tests PRIVATE include )
  add_executable( bicycle-tests tests/codec_test.cpp )
  target_link_libraries( bicycle-tests PRIVATE bicycle_pack_codecs )
  enable_testing()
  # The pack round-trip needs test/assets.bpak, which dev builds don't make: pack it first.
  add_test( NAME pack-assets COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target package )
  set_tests_properties( pack-assets PROPERTIES FIXTURES_SETUP packed )
  add_test( NAME codecs COMMAND bicycle-tests test WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" )
  add_test( NAME motion COMMAND bicycle-motion-tests )
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
  src/m/Entity.cpp src/m/Box.cpp src/m/Camera.cpp src/m/World.cpp
  src/v/common/Image.cpp src/v/common/ColorPalette.cpp src/v/common/Window.cpp src/v/common/Dialogue.cpp
  src/v/common/Menu.cpp src/v/common/TextMenu.cpp src/v/common/Scene.cpp src/v/common/Bar.cpp
  src/c/Trigger.cpp src/c/Timer.cpp src/c/WindowManager.cpp src/c/CollisionDetector.cpp
  src/c/Kinematics.cpp src/c/Animator.cpp src/c/MidiPlayer.cpp src/c/Dice.cpp
  src/c/Physics3D.cpp src/c/World3DApi.cpp src/c/InputState.cpp )
# Not built (unfinished or replaced): src/c/Input.cpp (Linux evdev keyboard), src/c/RenderSystem.cpp, src/v/common/Graph.cpp
if ( BICYCLE_RENDER STREQUAL "ASCII" )
  list( APPEND BICYCLE_SOURCES src/v/ascii/AsciiRenderer.cpp src/c/SshInputListener.cpp )
elseif ( BICYCLE_RENDER STREQUAL "2D" )
  list( APPEND BICYCLE_SOURCES src/v/sdl/PixelRenderer.cpp src/v/sdl/UiFont.cpp src/v/sdl/SpriteAtlas.cpp src/v/2d/Renderer2D.cpp )
elseif ( BICYCLE_RENDER STREQUAL "3D" )
  list( APPEND BICYCLE_SOURCES src/v/sdl/PixelRenderer.cpp src/v/sdl/UiFont.cpp src/v/sdl/SpriteAtlas.cpp src/v/3d/Renderer3D.cpp )
endif()

if ( BICYCLE_RELEASE )
  list( APPEND BICYCLE_SOURCES src/data/EmbeddedPack.c )
  set_source_files_properties( src/data/EmbeddedPack.c PROPERTIES
    COMPILE_DEFINITIONS "BICYCLE_EMBEDDED_PACK=\"${BICYCLE_EMBEDDED_PACK}\""
    OBJECT_DEPENDS "${BICYCLE_EMBEDDED_PACK}" )
endif()
add_executable( o ${BICYCLE_SOURCES} )
target_include_directories( o PRIVATE include )
target_link_libraries( o PRIVATE bicycle_data bicycle_luajit bicycle_fluidsynth Threads::Threads )
target_compile_definitions( o PRIVATE BICYCLE_RENDER_${BICYCLE_RENDER} )
if ( BICYCLE_RELEASE_ASSETS )
  target_compile_definitions( o PRIVATE BICYCLE_RELEASE_ASSETS )
endif()
if ( BICYCLE_RELEASE )
  target_compile_definitions( o PRIVATE BICYCLE_RELEASE "BICYCLE_RELEASE_GAME=\"${BICYCLE_RELEASE_GAME}\"" )
  target_compile_definitions( bicycle_data PRIVATE BICYCLE_RELEASE )
  add_dependencies( o embedded-pack )
endif()
if ( BICYCLE_RENDER STREQUAL "ASCII" )
  target_link_libraries( o PRIVATE bicycle_curses )
else()
  target_link_libraries( o PRIVATE bicycle_sdl3 )
endif()
# The executable is o (ASCII), o-2d or o-3d, so builds of different modes can sit side by side in the repo root.
set( BICYCLE_EXE_NAME o )
if ( NOT BICYCLE_RENDER STREQUAL "ASCII" )
  string( TOLOWER "o-${BICYCLE_RENDER}" BICYCLE_EXE_NAME )
endif()
set_target_properties( o PROPERTIES
  OUTPUT_NAME ${BICYCLE_EXE_NAME}
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_SOURCE_DIR}
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

# Release builds: run the game headless for 120 frames under strace and fail if it opens anything but OS files
# (tests/release_io_test.sh lists what's allowed and why). Needs strace; 2D/3D also xvfb-run (or a DISPLAY).
if ( BICYCLE_RELEASE AND NOT CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  string( TOLOWER "${BICYCLE_RENDER}" modeLower )
  add_test( NAME release-zero-io COMMAND sh "${CMAKE_SOURCE_DIR}/tests/release_io_test.sh" "$<TARGET_FILE:o>" ${modeLower} 120 )
  set_tests_properties( release-zero-io PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300 )
endif()
