#include "data/BImg.h"
#include "Bytes.h"

auto BImg::parse( std::string_view bytes ) -> std::optional<BImg> {
  bytes::Reader r( bytes );
  if ( r.str( 4 ) != "BIMG" ) return std::nullopt;
  BImg img;
  img.w = r.u16();
  img.h = r.u16();
  const int n = r.u16();
  r.u16();
  const size_t count = static_cast<size_t>( img.w ) * img.h;
  if ( n > 0 ) {
    img.palette.resize( n );
    for ( auto& c : img.palette ) c = r.u32();
    if ( !r.need( count ) ) return std::nullopt;
    img.indices.assign( bytes.begin() + r.pos, bytes.begin() + r.pos + count );
    for ( auto i : img.indices ) {
      if ( i >= n ) return std::nullopt;
    }
  }
  else {
    img.rgba.resize( count );
    for ( auto& c : img.rgba ) c = r.u32();
  }
  if ( !r.ok ) return std::nullopt;
  return img;
}
