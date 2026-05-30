#pragma once
#include <cursesw.h>
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <yaml-cpp/yaml.h>
#include <variant>

#include "bicycle.h"
#include "Config.h"
#include "Constants.h"
#include "m/Blackboard.h"
#include "c/Timer.h"
#include "c/InputData.h"

// TODO resolve these to real datatypes
using CollisionData = int;
using TimerData = double;

enum class ActionState { READY, FAILED, PAUSED, IN_PROGRESS, COMPLETE };
//
// Function template
template<typename T>
using Cb = std::function<ActionState(Blackboard&, T&)>;
using ActCallback = std::variant< Cb<InputState>, Cb<CollisionData>, Cb<TimerData> >;


#define I(_iname_) ActionState _iname_ ( Blackboard& bb, InputState& input )
// TODO #define T(_tname_) ActionState _tname_ ( Blackboard& bb, TimerData )
// TODO #define C(_cname_) ActionState _cname_ ( Blackboard& bb, CollisionData )

// This is how we export the game's functions across the binary barrier.
template<typename T>
struct ActPkg {
  std::string name;
  ActCallback func;  // TODO can we get variant functions across the border?
};

#define IACT( _funcName_ ) InputActPkg{ std::string(#_funcName_), _funcName_ }
// TODO #define TACT( _funcName_ ) InputActPkg{ std::string(#_funcName_), _funcName_ }
// TODO #define CACT( _funcName_ ) InputActPkg{ std::string(#_funcName_), _funcName_ }


struct Quirk {
  ActCallback action{};
  int priority{};  // higher values take precedence
  unsigned freq{};  // freq at quirk-level gives entities more ownership over their own rates
  unsigned reps{};  // reps at quirk-level gives entities more ownership over their own repetitions
};

using QKey = std::string;  // LATER i might want to make this more minute like an enum.
using Quirks = std::map< QKey, Quirk >;
using Personality = Quirks;

// =======================================================
// ********************** YAML ***************************
// =======================================================

// Provide yaml-cpp library with template candidate for Quirk's specific struct
template<>
struct YAML::convert<Quirk> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Quirk& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    rhs.priority = node["priority"].as<decltype(Quirk::priority)>();
    if ( auto freq = node["freq"] ) {
      rhs.freq = freq.as<decltype(Quirk::freq)>();
    }
    if ( auto freq = node["reps"] ) {
      rhs.freq = freq.as<decltype(Quirk::reps)>();
    }
    return true;
  }
};   // Quirk YML conversion

// Provide yaml-cpp library with template candidate for Quirk's specific struct
template<>
struct YAML::convert<Personality> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Personality& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    rhs =  node.as<Personality>();
    return true;
  }
};   // Personality YML conversion

