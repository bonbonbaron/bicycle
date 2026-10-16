#include "StripEncode.h"
#include "Deflate.h"
#include "../../src/data/Bytes.h"
#include <map>
#include <stdexcept>
#include <vector>

auto stripEncode( const BImg& img, int n ) -> std::string {
  if ( !img.indexed() ) throw std::runtime_error( "strip encoding needs an indexed image" );
  const int nColors = static_cast<int>( img.palette.size() );
  const int bpu = nColors <= 2 ? 1 : nColors <= 4 ? 2 : nColors <= 16 ? 4 : 8;
  const size_t count = img.indices.size();
  const size_t nIdx = ( count + n - 1 ) / n;
  // Deduplicate strips (the last one is padded with index 0).
  std::map<std::vector<uint8_t>, uint16_t> seen;
  std::vector<uint8_t> units;
  std::string sm;
  for ( size_t k = 0; k < nIdx; ++k ) {
    std::vector<uint8_t> s( n, 0 );
    for ( int u = 0; u < n; ++u ) {
      if ( k * n + u < count ) s[u] = img.indices[k * n + u];
    }
    auto it = seen.find( s );
    if ( it == seen.end() ) {
      if ( seen.size() >= 65535 ) throw std::runtime_error( "too many distinct strips" );
      it = seen.emplace( s, static_cast<uint16_t>( seen.size() ) ).first;
      units.insert( units.end(), s.begin(), s.end() );
    }
    bytes::u16( sm, it->second );
  }
  // Bit-pack, LSB first.
  const int perByte = 8 / bpu;
  std::string ss( ( units.size() + perByte - 1 ) / perByte, '\0' );
  for ( size_t i = 0; i < units.size(); ++i ) {
    ss[i / perByte] = static_cast<char>( static_cast<uint8_t>( ss[i / perByte] ) | ( units[i] << ( ( i % perByte ) * bpu ) ) );
  }
  const std::string ssZ = deflateZlib( ss ), smZ = deflateZlib( sm );
  std::string o = "STRP";
  bytes::u16( o, img.w );
  bytes::u16( o, img.h );
  bytes::u16( o, nColors );
  bytes::u8( o, n );
  bytes::u8( o, bpu );
  bytes::u32( o, seen.size() );
  bytes::u32( o, nIdx );
  for ( auto c : img.palette ) bytes::u32( o, c );
  bytes::u32( o, ss.size() );
  bytes::u32( o, ssZ.size() );
  bytes::u32( o, sm.size() );
  bytes::u32( o, smZ.size() );
  return o + ssZ + smZ;
}
