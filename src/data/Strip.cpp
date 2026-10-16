#include "data/Strip.h"
#include "data/Pack.h"
#include "Bytes.h"

namespace strip {

auto decode( const std::string& enc ) -> std::optional<std::string> {
  bytes::Reader r( enc );
  if ( r.str( 4 ) != "STRP" ) return std::nullopt;
  BImg img;
  img.w = r.u16();
  img.h = r.u16();
  const int nColors = r.u16();
  const int unitsPerStrip = r.u8();
  const int bpu = r.u8();
  const uint32_t nStrips = r.u32();
  const uint32_t nIndices = r.u32();
  img.palette.resize( nColors );
  for ( auto& c : img.palette ) c = r.u32();
  const uint32_t ssRaw = r.u32(), ssComp = r.u32(), smRaw = r.u32(), smComp = r.u32();
  if ( !r.ok || nColors == 0 || unitsPerStrip == 0 || !( bpu == 1 || bpu == 2 || bpu == 4 || bpu == 8 ) ) return std::nullopt;
  if ( !r.need( ssComp + static_cast<size_t>( smComp ) ) ) return std::nullopt;
  // 1. Inflate the stripset and stripmap.
  auto ss = assets::inflateZlib( reinterpret_cast<const uint8_t*>( enc.data() + r.pos ), ssComp, ssRaw );
  auto sm = assets::inflateZlib( reinterpret_cast<const uint8_t*>( enc.data() + r.pos + ssComp ), smComp, smRaw );
  if ( !ss || !sm || sm->size() != nIndices * 2u ) return std::nullopt;
  // 2. Unpack the stripset's units (LSB first).
  const size_t nUnits = static_cast<size_t>( nStrips ) * unitsPerStrip;
  if ( ss->size() * 8 < nUnits * bpu ) return std::nullopt;
  std::vector<uint8_t> units( nUnits );
  const uint8_t mask = static_cast<uint8_t>( ( 1u << bpu ) - 1 );
  const int perByte = 8 / bpu;
  for ( size_t i = 0; i < nUnits; ++i ) {
    const uint8_t byte = static_cast<uint8_t>( ( *ss )[i / perByte] );
    units[i] = ( byte >> ( ( i % perByte ) * bpu ) ) & mask;
  }
  // 3. Assemble: lay the strips out in stripmap order.
  const size_t count = static_cast<size_t>( img.w ) * img.h;
  if ( static_cast<size_t>( nIndices ) * unitsPerStrip < count ) return std::nullopt;
  img.indices.resize( count );
  for ( uint32_t k = 0; k < nIndices; ++k ) {
    const uint16_t s = static_cast<uint8_t>( ( *sm )[2 * k] ) | ( static_cast<uint8_t>( ( *sm )[2 * k + 1] ) << 8 );
    if ( s >= nStrips ) return std::nullopt;
    for ( int u = 0; u < unitsPerStrip; ++u ) {
      const size_t dst = static_cast<size_t>( k ) * unitsPerStrip + u;
      if ( dst < count ) {
        const uint8_t idx = units[static_cast<size_t>( s ) * unitsPerStrip + u];
        if ( idx >= nColors ) return std::nullopt;
        img.indices[dst] = idx;
      }
    }
  }
  return img.serialize();
}

}  // namespace strip
