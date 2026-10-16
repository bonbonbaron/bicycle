/* Release builds: the game's asset pack (main.lua, genomes, images, meshes, MIDI, SoundFonts) plus the engine's
 * Lua scripts and UI fonts, linked into the executable so the engine never opens a file (see include/data/Pack.h).
 * BICYCLE_EMBEDDED_PACK is the .bpak's absolute path; CMake rebuilds this object when the pack changes.
 * .incbin works with GCC and clang on ELF (Linux) and COFF (MinGW, llvm-mingw). */
#ifndef BICYCLE_EMBEDDED_PACK
#error "BICYCLE_EMBEDDED_PACK must name the .bpak to embed"
#endif
#if defined( _WIN32 )
#define BICYCLE_RODATA ".section .rdata,\"dr\"\n"
#elif defined( __APPLE__ )
#define BICYCLE_RODATA ".const_data\n"
#else
#define BICYCLE_RODATA ".section .rodata\n"
#endif
#if defined( __APPLE__ ) || ( defined( _WIN32 ) && defined( __i386__ ) )
#define BICYCLE_SYM( s ) "_" s
#else
#define BICYCLE_SYM( s ) s
#endif

__asm__(
  BICYCLE_RODATA
  ".balign 16\n"
  ".globl " BICYCLE_SYM( "bicycle_embedded_pack" ) "\n"
  BICYCLE_SYM( "bicycle_embedded_pack" ) ":\n"
  ".incbin \"" BICYCLE_EMBEDDED_PACK "\"\n"
  ".globl " BICYCLE_SYM( "bicycle_embedded_pack_end" ) "\n"
  BICYCLE_SYM( "bicycle_embedded_pack_end" ) ":\n"
  ".byte 0\n"
  ".text\n" );
