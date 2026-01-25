#pragma once
#include "Body.h"
#include "Personality.h"

struct Entity {
  Body body;
  Personality personality;
};


// Provide yaml-cpp library with template candidate for Quirk's specific struct
template<>
struct YAML::convert<Entity> {
  static YAML::Node encode(const Image& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Entity& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    // We don't set pos here. Why would a body have a permanent position? :) We'll make that easy later.
    try {
      auto bodyName = node["body"].as<std::string>();
      auto persName = node["personality"].as<std::string>();
      auto bodyRootNode = YAML::LoadFile( BODY_DIR.data() + bodyName + SUFFIX.data() );
      auto persRootNode = YAML::LoadFile( PERS_DIR.data() + persName + SUFFIX.data() );
      rhs.body = bodyRootNode.as<Body>();
      rhs.personality = persRootNode.as<Personality>();
    }
    catch ( const std::invalid_argument &e ) {
      throw e;  // All this throwing just makes a big baseball game at this point.
    }
    catch ( const YAML::Exception& e ) {
      throw e;
    }
    return true;
  }
};   // Entity YML conversion

//TODO yaml
