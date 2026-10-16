#include "data/BImg.h"
#include "Bytes.h"

auto BImg::parse( const std::string& bytes ) -> std::optional<BImg> {
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

auto BImg::serialize() const -> std::string {
  std::string o = "BIMG";
  bytes::u16( o, w );
  bytes::u16( o, h );
  bytes::u16( o, palette.size() );
  bytes::u16( o, 0 );
  if ( indexed() ) {
    for ( auto c : palette ) bytes::u32( o, c );
    o.append( reinterpret_cast<const char*>( indices.data() ), indices.size() );
  }
  else {
    for ( auto c : rgba ) bytes::u32( o, c );
  }
  return o;
}
