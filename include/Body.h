#pragma once
#include <string>
#include "ColorPalette.h"
#include "Config.h"
#include <yaml-cpp/node/convert.h>
struct Position {
  Position() = default;
  Position( const Position& ) = default;
  Position( Position&& ) = default;
  Position& operator=( const Position& ) = default;
  Position& operator=( Position&& ) = default;
  int x{};
  int y{};

  void operator+=( const Position& rhs ) {
    x += rhs.x;
    y += rhs.y;
  }

  auto operator+( const Position& rhs ) -> Position {
    Position pos;
    pos.x = x + rhs.x;
    pos.y = y + rhs.y;
    return pos;
  }
};

using Image = std::string;

// Body is the visual representation of an entity.
class Body {
  public:
    auto getPosition() const -> const std::shared_ptr<Position>&;
    void setPosition( const std::shared_ptr<Position>& pos );

    auto getColor() const -> Color;
    void setColor( const std::string& color );
    void setColor( const Color& color );

    auto getSymbol() const -> const Image&;
    void setSymbol( const Image& sym );
    void setSymbol( const Image&& sym );

  private:
    Image _sym;  
    std::shared_ptr<Position> _pos{};  // shared with blackboards 
    Color _color{};
};


// ******************************
// YAML Conversions
// ******************************

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

template<>
struct YAML::convert<Position> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Position& rhs) {
    if (!node.IsSequence()) {
      return false;
    }

    // Ensure it's a sequence of 2 elements.
    if ( node.size() != 2 ) {
      return false;
    }

    // Ensure they're both integers.
    rhs.x = node[0].as<int>();
    rhs.y = node[1].as<int>();

    return true;
  }
};

