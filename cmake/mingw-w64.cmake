# Cross-compiling o.exe from Linux with MinGW-w64 (Debian/Ubuntu: apt install mingw-w64), e.g.
#   cmake -B build-win -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake && cmake --build build-win
# Dependencies are fetched and built for Windows like on any other platform (see cmake/BicycleDeps.cmake).
# For llvm-mingw (clang) set MINGW_CC/MINGW_CXX, e.g. -DMINGW_CC=x86_64-w64-mingw32-clang -DMINGW_CXX=x86_64-w64-mingw32-clang++.
set( CMAKE_SYSTEM_NAME Windows )
set( CMAKE_SYSTEM_PROCESSOR x86_64 )
set( MINGW_CC x86_64-w64-mingw32-gcc-posix CACHE STRING "MinGW C compiler" )      # -posix: std::thread support
set( MINGW_CXX x86_64-w64-mingw32-g++-posix CACHE STRING "MinGW C++ compiler" )
if ( NOT CMAKE_C_COMPILER )  # dependency builds get the compilers passed explicitly
  set( CMAKE_C_COMPILER ${MINGW_CC} )
endif()
if ( NOT CMAKE_CXX_COMPILER )
  set( CMAKE_CXX_COMPILER ${MINGW_CXX} )
endif()
set( CMAKE_RC_COMPILER x86_64-w64-mingw32-windres )
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY )
# Don't let the host's pkg-config hand out Linux headers and libraries.
set( ENV{PKG_CONFIG_LIBDIR} "" )
set( ENV{PKG_CONFIG_PATH} "" )
