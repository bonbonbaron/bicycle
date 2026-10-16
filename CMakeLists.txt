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

include( cmake/BicycleDeps.cmake )
if ( BICYCLE_RENDER STREQUAL "ASCII" )
  bicycle_curses()
endif()
bicycle_luajit()
bicycle_fluidsynth()
bicycle_gm_soundfont()
find_package( Threads REQUIRED )

set( BICYCLE_SOURCES
  src/main.cpp src/bicycle.cpp src/trigLUT.cpp
  src/m/Entity.cpp src/m/Box.cpp src/m/Camera.cpp src/m/World.cpp
  src/v/common/Image.cpp src/v/common/ColorPalette.cpp src/v/common/Window.cpp src/v/common/Dialogue.cpp
  src/v/common/Menu.cpp src/v/common/TextMenu.cpp src/v/common/Scene.cpp src/v/common/Bar.cpp
  src/c/Trigger.cpp src/c/Timer.cpp src/c/WindowManager.cpp src/c/CollisionDetector.cpp
  src/c/Kinematics.cpp src/c/Animator.cpp src/c/MidiPlayer.cpp src/c/Dice.cpp )
# Not built (unfinished or replaced): src/c/Input.cpp (Linux evdev keyboard), src/c/RenderSystem.cpp, src/v/common/Graph.cpp
if ( BICYCLE_RENDER STREQUAL "ASCII" )
  list( APPEND BICYCLE_SOURCES src/v/ascii/AsciiRenderer.cpp src/c/SshInputListener.cpp )
endif()

add_executable( o ${BICYCLE_SOURCES} )
target_include_directories( o PRIVATE include )
target_link_libraries( o PRIVATE bicycle_luajit bicycle_fluidsynth Threads::Threads )
target_compile_definitions( o PRIVATE BICYCLE_RENDER_${BICYCLE_RENDER} )
if ( BICYCLE_RENDER STREQUAL "ASCII" )
  target_link_libraries( o PRIVATE bicycle_curses )
endif()
set_target_properties( o PROPERTIES
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
