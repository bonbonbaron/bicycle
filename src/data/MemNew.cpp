// Routes every operator new/delete in the engine (std containers, strings, shared_ptr, ...) into the GENERAL pool.
// Linked into the engine only (tools and tests use the system heap, plus pools where they ask for them).
#include "data/Mem.h"
#include <cstdint>

namespace {
auto alignedAdd( std::size_t n, std::size_t align ) -> void* {
  if ( align <= 16 ) return mem::memAdd( n, mem::GENERAL );
  // Over-allocate, align, and remember the real block just before the aligned pointer.
  auto* raw = static_cast<uint8_t*>( mem::memAdd( n + align + sizeof( void* ), mem::GENERAL ) );
  auto addr = ( reinterpret_cast<uintptr_t>( raw ) + sizeof( void* ) + align - 1 ) & ~( uintptr_t( align ) - 1 );
  reinterpret_cast<void**>( addr )[-1] = raw;
  return reinterpret_cast<void*>( addr );
}
void alignedDel( void* p, std::size_t align ) {
  if ( !p ) return;
  mem::memDel( align <= 16 ? p : reinterpret_cast<void**>( p )[-1] );
}
}  // namespace

void* operator new( std::size_t n ) { return mem::memAdd( n, mem::GENERAL ); }
void* operator new[]( std::size_t n ) { return mem::memAdd( n, mem::GENERAL ); }
void* operator new( std::size_t n, const std::nothrow_t& ) noexcept { return mem::memAdd( n, mem::GENERAL ); }
void* operator new[]( std::size_t n, const std::nothrow_t& ) noexcept { return mem::memAdd( n, mem::GENERAL ); }
void operator delete( void* p ) noexcept { mem::memDel( p ); }
void operator delete[]( void* p ) noexcept { mem::memDel( p ); }
void operator delete( void* p, std::size_t ) noexcept { mem::memDel( p ); }
void operator delete[]( void* p, std::size_t ) noexcept { mem::memDel( p ); }
void operator delete( void* p, const std::nothrow_t& ) noexcept { mem::memDel( p ); }
void operator delete[]( void* p, const std::nothrow_t& ) noexcept { mem::memDel( p ); }
void* operator new( std::size_t n, std::align_val_t a ) { return alignedAdd( n, static_cast<std::size_t>( a ) ); }
void* operator new[]( std::size_t n, std::align_val_t a ) { return alignedAdd( n, static_cast<std::size_t>( a ) ); }
void operator delete( void* p, std::align_val_t a ) noexcept { alignedDel( p, static_cast<std::size_t>( a ) ); }
void operator delete[]( void* p, std::align_val_t a ) noexcept { alignedDel( p, static_cast<std::size_t>( a ) ); }
void operator delete( void* p, std::size_t, std::align_val_t a ) noexcept { alignedDel( p, static_cast<std::size_t>( a ) ); }
void operator delete[]( void* p, std::size_t, std::align_val_t a ) noexcept { alignedDel( p, static_cast<std::size_t>( a ) ); }
