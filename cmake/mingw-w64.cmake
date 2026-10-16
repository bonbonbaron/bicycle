# Cross-compiling o.exe from Linux with MinGW-w64 (Debian/Ubuntu: apt install mingw-w64). Put the Windows builds of
# PDCurses, LuaJIT and fluidsynth under ~/win64 (include/, lib/, bin/) or pass -DCMAKE_PREFIX_PATH. See README.md.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32 $ENV{HOME}/win64)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
# Don't let the host's pkg-config hand out Linux headers and libraries; only look at the Windows prefix.
set(ENV{PKG_CONFIG_LIBDIR} "$ENV{HOME}/win64/lib/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "")
