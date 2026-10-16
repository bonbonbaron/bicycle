#include "Deflate.h"
#include "miniz.h"
#include <stdexcept>

auto deflateZlib( const std::string& raw ) -> std::string {
  mz_ulong len = mz_compressBound( raw.size() );
  std::string out( len, '\0' );
  if ( mz_compress2( reinterpret_cast<unsigned char*>( out.data() ), &len,
                     reinterpret_cast<const unsigned char*>( raw.data() ), raw.size(), MZ_UBER_COMPRESSION ) != MZ_OK ) {
    throw std::runtime_error( "deflate failed" );
  }
  out.resize( len );
  return out;
}
