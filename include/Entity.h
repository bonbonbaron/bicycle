#pragma once
#include "Body.h"
#include "Personality.h"
#include "Blackboard.h"

struct Entity {
  Body body{};
  Personality personality{};
  std::shared_ptr<Blackboard> bb{};  // this is shared with action nodes

  void validate();
};


// Provide yaml-cpp library with template candidate for Quirk's specific struct
template<>
struct YAML::convert<Entity> {
  static YAML::Node encode(const Image& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Entity& rhs) {
    std::string bbName;

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

      // Blackboards are optional for stateless entities.
      if ( auto& bbNode = node["bb"] ) {
        bbName = bbNode.as<std::string>();
        auto& reg = BlackboardRegistry::getInstance();
        rhs.bb = reg.at( bbName );
        rhs.personality.distributeBlackboard( rhs.bb );
        try {
          rhs.personality.validate();
        }
        catch ( const std::runtime_error& e ) {
          std::cerr << "Error validating blackboard \'" << bbName << "\' against port set\n";
          throw e;
        }
      }
    }
    catch ( const std::out_of_range &e ) {
      std::cerr << "Couldn't find a blackboard by the name of '" << bbName << "\'.\n";
      throw e;  // All this throwing just makes a big baseball game at this point.
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
