#include "data/Strip.h"
#include "data/Pack.h"
#include "Bytes.h"

namespace strip {

auto decode( std::string_view enc, mem::PoolId pool ) -> std::optional<mem::Blob> {
  bytes::Reader r( enc );
  if ( r.str( 4 ) != "STRP" ) return std::nullopt;
  const uint16_t w = r.u16(), h = r.u16();
  const int nColors = r.u16();
  const int unitsPerStrip = r.u8();
  const int bpu = r.u8();
  const uint32_t nStrips = r.u32();
  const uint32_t nIndices = r.u32();
  const std::string_view palette = r.str( 4 * static_cast<std::size_t>( nColors ) );  // RGBA u32s, as in .bimg
  const uint32_t ssRaw = r.u32(), ssComp = r.u32(), smRaw = r.u32(), smComp = r.u32();
  if ( !r.ok || nColors == 0 || unitsPerStrip == 0 || !( bpu == 1 || bpu == 2 || bpu == 4 || bpu == 8 ) ) return std::nullopt;
  if ( !r.need( ssComp + static_cast<std::size_t>( smComp ) ) ) return std::nullopt;
  const std::size_t count = static_cast<std::size_t>( w ) * h;
  if ( static_cast<std::size_t>( nIndices ) * unitsPerStrip < count ) return std::nullopt;
  // 1. Inflate the stripset and stripmap.
  auto ss = assets::inflateZlib( reinterpret_cast<const uint8_t*>( enc.data() + r.pos ), ssComp, ssRaw, mem::TEMPORARY );
  auto sm = assets::inflateZlib( reinterpret_cast<const uint8_t*>( enc.data() + r.pos + ssComp ), smComp, smRaw, mem::TEMPORARY );
  if ( !ss || !sm || sm->size() != nIndices * 2u ) return std::nullopt;
  // 2. Unpack the stripset's units (LSB first).
  const std::size_t nUnits = static_cast<std::size_t>( nStrips ) * unitsPerStrip;
  if ( ss->size() * 8 < nUnits * bpu ) return std::nullopt;
  mem::Vector<uint8_t, mem::TEMPORARY> units( nUnits );
  const uint8_t mask = static_cast<uint8_t>( ( 1u << bpu ) - 1 );
  const int perByte = 8 / bpu;
  for ( std::size_t i = 0; i < nUnits; ++i ) {
    const uint8_t byte = static_cast<uint8_t>( ( *ss )[i / perByte] );
    units[i] = ( byte >> ( ( i % perByte ) * bpu ) ) & mask;
  }
  // 3. Assemble the .bimg (include/data/BImg.h) in place: header, the palette as stored, then the strips laid
  //    out in stripmap order.
  const std::size_t hdr = 12 + palette.size();
  mem::Blob out( hdr + count, '\0', mem::Allocator<char>( pool ) );
  const uint16_t head[4] = { w, h, static_cast<uint16_t>( nColors ), 0 };
  out.replace( 0, 4, "BIMG" );
  for ( int i = 0; i < 4; ++i ) {
    out[4 + 2 * i] = static_cast<char>( head[i] & 0xff );
    out[5 + 2 * i] = static_cast<char>( head[i] >> 8 );
  }
  out.replace( 12, palette.size(), palette.data(), palette.size() );
  for ( uint32_t k = 0; k < nIndices; ++k ) {
    const uint16_t s = static_cast<uint8_t>( ( *sm )[2 * k] ) | ( static_cast<uint8_t>( ( *sm )[2 * k + 1] ) << 8 );
    if ( s >= nStrips ) return std::nullopt;
    for ( int u = 0; u < unitsPerStrip; ++u ) {
      const std::size_t dst = static_cast<std::size_t>( k ) * unitsPerStrip + u;
      if ( dst < count ) {
        const uint8_t idx = units[static_cast<std::size_t>( s ) * unitsPerStrip + u];
        if ( idx >= nColors ) return std::nullopt;
        out[hdr + dst] = static_cast<char>( idx );
      }
    }
  }
  return out;
}

}  // namespace strip
