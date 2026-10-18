# Cross-compile for 32-bit Raspberry Pi OS on the original Pi Zero / Zero W (ARMv6 ARM1176, hard float, glibc 2.36) with zig.
# Wrapper scripts (ZIG_WRAP dir) are needed because CMake wants a single compiler path:
#   arm6-cc:  exec zig cc  -target arm-linux-gnueabihf.2.36 -mcpu=arm1176jzf_s -mfloat-abi=hard "$@"   (likewise arm6-c++, arm6-ar, arm6-ranlib)
# cmake -B build-pi -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/armv6-zig.cmake -DZIG_WRAP=/path/to/wrappers
set( CMAKE_SYSTEM_NAME Linux )
set( CMAKE_SYSTEM_PROCESSOR arm )
set( ZIG_WRAP "$ENV{ZIG_WRAP}" CACHE PATH "Directory with the arm6-cc/arm6-c++/arm6-ar/arm6-ranlib wrappers" )
set( CMAKE_C_COMPILER "${ZIG_WRAP}/arm6-cc" )
set( CMAKE_CXX_COMPILER "${ZIG_WRAP}/arm6-c++" )
set( CMAKE_AR "${ZIG_WRAP}/arm6-ar" CACHE FILEPATH "" )
set( CMAKE_RANLIB "${ZIG_WRAP}/arm6-ranlib" CACHE FILEPATH "" )
set( CMAKE_CROSSCOMPILING_EMULATOR qemu-arm -L /usr/arm-linux-gnueabihf )
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY )
# LuaJIT's buildvm must match the target's pointer size: build it with a 32-bit host compiler (gcc-multilib).
set( BICYCLE_LUAJIT_HOST_CC "gcc -m32" CACHE STRING "" )
