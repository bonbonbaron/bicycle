#pragma once
#include <optional>
#include <string_view>
#include "data/Map.h"
#include "gen/ids.h"

enum Color {
  BLACK	  ,
  RED	    ,
  GREEN	  ,
  YELLOW  ,
  BLUE	  ,
  MAGENTA ,
  CYAN	  ,
  WHITE   ,
};

// Colors by color id (ids::Color, generated from the enum above by tools/ids/genids.lua): the old
// string-keyed strToColor. Names are resolved once (colorId) and then looked up by id.
inline auto colorById() -> const bicycle::map<Color>& {
  static const bicycle::map<Color> m = [] {
    bicycle::map<Color> c( ids::colorCount, mem::PERMANENT );
    for ( unsigned i = 0; i < ids::colorCount; ++i ) c.set( static_cast<uint8_t>( i ), static_cast<Color>( i ) );
    return c;
  }();
  return m;
}
// A color name ("RED") -> its id, at load time; nullopt for an unknown name.
inline auto colorId( std::string_view name ) -> std::optional<uint8_t> {
  for ( unsigned i = 0; i < ids::colorCount; ++i ) {
    if ( ids::colorNames[i] && name == ids::colorNames[i] ) return static_cast<uint8_t>( i );
  }
  return std::nullopt;
}
inline auto strToColor( std::string_view name ) -> std::optional<Color> {
  auto id = colorId( name );
  if ( !id ) return std::nullopt;
  const Color* c = colorById().find( *id );
  return c ? std::optional<Color>( *c ) : std::nullopt;
}

class ColorPalette {
  private:
    ColorPalette() = default;
    ColorPalette( const ColorPalette& ) = delete;
    auto operator=( const ColorPalette& ) -> ColorPalette& = delete;
  public:
    static auto getInstance() -> const ColorPalette& {
      const static ColorPalette cp;
      return cp;
    }
};


