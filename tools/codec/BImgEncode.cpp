// .bimg writing (offline: bicycle-pack and the codec tests). The engine only parses .bimg (src/data/BImg.cpp).
#include "codec/BImgEncode.h"
#include "../../src/data/Bytes.h"

namespace bimg {

auto serialize( const BImg& img ) -> std::string {
  std::string o = "BIMG";
  bytes::u16( o, img.w );
  bytes::u16( o, img.h );
  bytes::u16( o, img.palette.size() );
  bytes::u16( o, 0 );
  if ( img.indexed() ) {
    for ( auto c : img.palette ) bytes::u32( o, c );
    o.append( reinterpret_cast<const char*>( img.idx().data() ), img.idx().size() );
  }
  else {
    for ( auto c : img.rgba ) bytes::u32( o, c );
  }
  return o;
}

}  // namespace bimg
