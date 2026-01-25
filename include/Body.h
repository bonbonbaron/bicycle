#pragma once
#include <string>
#include "ColorPalette.h"
#include "Config.h"
#include <yaml-cpp/node/convert.h>
struct Position {
  int x{};
  int y{};
};

using Image = std::string;

// Body is the visual representation of an entity.
class Body {
  public:
    auto getPosition() const -> const Position&;
    void setPosition( const int x, const int y );

    auto getColor() const -> Color;
    void setColor( const std::string& color );

    auto getSymbol() const -> const Image&;
    void setSymbol( const Image& sym );
    void setSymbol( const Image&& sym );
  private:
    Image _sym;  
    Position _pos{};
    Color _color{};
};


// Provide yaml-cpp library with template candidate for Quirk's specific struct
template<>
struct YAML::convert<Body> {
  static YAML::Node encode(const Image& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Body& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    // We don't set pos here. Why would a body have a permanent position? :) We'll make that easy later.
    try {
      rhs.setSymbol( node["sym"].as<Image>() );
      rhs.setColor( node["color"].as<std::string>() );
    }
    catch ( const std::invalid_argument &e ) {
      throw e;  // All this throwing just makes a big baseball game at this point.
    }
    catch ( const YAML::Exception& e ) {
      throw e;
    }
    return true;
  }
};   // Body YML conversion

//TODO yaml
