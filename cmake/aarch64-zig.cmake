# Cross-compile for 64-bit Raspberry Pi OS (aarch64, glibc 2.36 = bookworm) with zig as the C/C++ compiler.
# Wrapper scripts (ZIG_WRAP dir) are needed because CMake wants a single compiler path:
#   a64-cc:  exec zig cc  -target aarch64-linux-gnu.2.36 -mcpu=cortex_a53 "$@"   (likewise a64-c++, a64-ar, a64-ranlib)
# cmake -B build-pi -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-zig.cmake -DZIG_WRAP=/path/to/wrappers
set( CMAKE_SYSTEM_NAME Linux )
set( CMAKE_SYSTEM_PROCESSOR aarch64 )
set( ZIG_WRAP "$ENV{ZIG_WRAP}" CACHE PATH "Directory with the a64-cc/a64-c++/a64-ar/a64-ranlib wrappers" )
set( CMAKE_C_COMPILER "${ZIG_WRAP}/a64-cc" )
set( CMAKE_CXX_COMPILER "${ZIG_WRAP}/a64-c++" )
set( CMAKE_AR "${ZIG_WRAP}/a64-ar" CACHE FILEPATH "" )
set( CMAKE_RANLIB "${ZIG_WRAP}/a64-ranlib" CACHE FILEPATH "" )
set( CMAKE_CROSSCOMPILING_EMULATOR qemu-aarch64 -L /usr/aarch64-linux-gnu )
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY )
