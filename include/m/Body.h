#pragma once
#include <string>
#include "v/ColorPalette.h"
#include "Config.h"
#include <yaml-cpp/node/convert.h>
#include "v/Image.h"

// Body is the visual representation of an entity.
class Body {
  // TODO should this just become a strucxt instead of a full-blown class?

  private:
    Image _img;  
    std::shared_ptr<Position> _pos{};  // shared with blackboards 
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
      rhs.setImage( node["img"].as<Image>() );
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

