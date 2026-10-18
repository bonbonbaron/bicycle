/* 32-bit ARM builds with zig/compiler-rt: LuaJIT's IR call table (lj_ir.c) names libgcc's 64-bit integer ->
   float helpers directly, and compiler-rt for ARM EABI only provides the __aeabi_* forms. The casts below
   compile to those. Only linked into 32-bit ARM builds (see CMakeLists.txt). */
#include <stdint.h>
double __floatdidf( int64_t x ) { return (double)x; }
float __floatdisf( int64_t x ) { return (float)x; }
double __floatundidf( uint64_t x ) { return (double)x; }
float __floatundisf( uint64_t x ) { return (float)x; }
