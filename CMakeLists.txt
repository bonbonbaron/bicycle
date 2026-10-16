# bicycle: builds the engine executable `o` (o.exe on Windows) into the repository root, next to the games it
# runs (`./o test` runs test/main.lua). Linux uses pkg-config (ncurses, luajit, fluidsynth); Windows uses vcpkg
# (pdcurses, luajit, fluidsynth; see vcpkg.json and README.md).
cmake_minimum_required( VERSION 3.20 )
project( bicycle CXX )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
if ( NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES )
  set( CMAKE_BUILD_TYPE Debug )  # same as the Makefile's -g
endif()

set( BICYCLE_SOURCES
  src/main.cpp src/bicycle.cpp src/trigLUT.cpp
  src/m/Entity.cpp src/m/Box.cpp src/m/Camera.cpp src/m/World.cpp
  src/v/Image.cpp src/v/ColorPalette.cpp src/v/Window.cpp src/v/Dialogue.cpp src/v/Menu.cpp src/v/TextMenu.cpp
  src/v/Scene.cpp src/v/Bar.cpp
  src/c/Trigger.cpp src/c/Timer.cpp src/c/SshInputListener.cpp src/c/WindowManager.cpp src/c/CollisionDetector.cpp
  src/c/Kinematics.cpp src/c/Animator.cpp src/c/MidiPlayer.cpp src/c/Dice.cpp )
# Not built (unfinished or replaced): src/c/Input.cpp (Linux evdev keyboard), src/c/RenderSystem.cpp, src/v/Graph.cpp

add_executable( o ${BICYCLE_SOURCES} )
target_include_directories( o PRIVATE include )
set_target_properties( o PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}
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
  target_link_options( o PRIVATE -Wl,--export-all-symbols )
endif()

find_package( Threads REQUIRED )
target_link_libraries( o PRIVATE Threads::Threads )

# Finds a library through pkg-config if there is one, else by header and library name (vcpkg, MSYS2, by hand).
find_package( PkgConfig QUIET )
function( bicycle_find_lib target pcName header libNames headerSuffixes )
  if ( PKG_CONFIG_FOUND )
    pkg_check_modules( ${target}_PC QUIET IMPORTED_TARGET ${pcName} )
    if ( ${target}_PC_FOUND )
      target_link_libraries( o PRIVATE PkgConfig::${target}_PC )
      message( STATUS "bicycle: ${pcName} from pkg-config" )
      return()
    endif()
  endif()
  find_path( ${target}_INCLUDE_DIR ${header} PATH_SUFFIXES ${headerSuffixes} )
  find_library( ${target}_LIBRARY NAMES ${libNames} )
  if ( NOT ${target}_INCLUDE_DIR OR NOT ${target}_LIBRARY )
    message( FATAL_ERROR "bicycle: couldn't find ${pcName} (${header}, ${libNames}). On Windows, use the vcpkg toolchain; see README.md." )
  endif()
  target_include_directories( o PRIVATE ${${target}_INCLUDE_DIR} )
  target_link_libraries( o PRIVATE ${${target}_LIBRARY} )
  message( STATUS "bicycle: ${pcName}: ${${target}_LIBRARY}" )
endfunction()

if ( WIN32 )
  # PDCurses (Windows console port); vcpkg's is "pdcurses". MSYS2's mingw-w64 pdcurses package works too.
  bicycle_find_lib( CURSES pdcurses curses.h "pdcurses;libpdcurses;pdcursesstatic" "pdcurses" )
else()
  bicycle_find_lib( CURSES ncurses curses.h "ncurses" "" )
endif()

bicycle_find_lib( LUAJIT luajit lua.hpp "luajit-5.1;lua51;luajit" "luajit;luajit-2.1;luajit-2.0" )

find_package( FluidSynth CONFIG QUIET )  # fluidsynth >= 2.4 (vcpkg) installs a CMake package
if ( TARGET FluidSynth::libfluidsynth )
  target_link_libraries( o PRIVATE FluidSynth::libfluidsynth )
  message( STATUS "bicycle: fluidsynth from its CMake package" )
else()
  bicycle_find_lib( FLUIDSYNTH fluidsynth fluidsynth.h "fluidsynth;libfluidsynth;libfluidsynth-3" "" )
endif()

# On Windows, vcpkg's toolchain copies the dependency DLLs (lua51.dll, libfluidsynth-3.dll, pdcurses.dll...) next
# to o.exe after each build (VCPKG_APPLOCAL_DEPS, on by default).
