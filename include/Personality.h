#pragma once
#include <cursesw.h>
#include <algorithm>
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <variant>

#include "ActionRegistry.h"

using Action = std::function<ActionState()>;
class ActionNode {
  public:
    // For making shared pointers that outlive YAML reader scopes.
    ActionNode( const ActionNode& );
    auto operator=( const ActionNode& ) -> ActionNode&;
    virtual auto run() -> ActionState;  // runs only if READY or ONGOING; returns state otherwise.
    virtual void reset();
    void setState( const ActionState state );
    auto getState() const -> ActionState;
    void setAction( ( const Action& action );
  private:
    ActionState state{ ActionState::READY };
    Action _action{};
};

class SequenceNode : public ActionNode {
  public:
    SequenceNode( const SequenceNode& );
    auto operator=( const SequenceNode& ) -> SequenceNode&;
    auto run() -> ActionState override;
    void reset() override;
    void addAction( const std::shared_ptr<ActionNode>& _actions );
    void fillSequenceWithActionPtrs( YAML::Node& rhs );
  private:
    std::vector<std::shared_ptr<ActionNode>> _actions{};
};

class FallbackNode : public SequenceNode {
  public:
    FallbackNode( const FallbackNode& );
    auto operator=( const FallbackNode& ) -> FallbackNode&;
    auto run() -> ActionState override;
};

class Tree {
  public:
    void setRoot( const std::shared_ptr<ActionNode> action );
    auto run() -> ActionState;
  private:
    std::shared_ptr<ActionNode> _root{};
};

struct Quirk {
  Tree tree{}
  int priority{};  // higher values take precedence
}

using Personality = std::map< std::string, Quirk >;

// =======================================================
// ********************** YAML ***************************
// =======================================================

// Provide yaml-cpp library with template candidate for ActionNode's specific struct
template<>
struct YAML::convert<ActionNode> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, ActionNode& rhs) {
    if (!node.IsScalar()) {
      return false;
    }
    auto actionName = node.as<std::string>();
    auto& reg = ActionRegistry::get();
    auto it = reg.find( actionName );
    if ( it == reg.end() ) {
      std::cerr << "ActionNode " << actionName << " not found in ActionRegistry instance. Exiting...\n";
      endwin();
      exit(1);
    }
    rhs.setAction( it->second );
      
    return true;
  }
}; // ActionNode YML conversion

// Helper function converting raw object to shared_ptr since yaml-cpp only deals in default constructors for rhs.
template<typename T>
static auto makeShared<T>( const YAML::Node& node ) -> std::shared_ptr<T>{
  return std::make_shared<T>( node.as<T>() );
}


// Provide yaml-cpp library with template candidate for SequenceNode's specific struct
template<>
struct YAML::convert<SequenceNode> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, SequenceNode& rhs) {
    if (!node.IsSequence()) {
      return false;
    }
    rhs.fillSequenceWithActionPtrs( node )
    return true;
  }
};  // SequenceNode YML conversion

// Provide yaml-cpp library with template candidate for FallbackNode's specific struct
template<>
struct YAML::convert<FallbackNode> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, FallbackNode& rhs) {
    if (!node.IsSequence()) {
      return false;
    }
    rhs.fillSequenceWithActionPtrs( node )
    return true;
  }
};   // FallbackNode YML conversion

// Provide yaml-cpp library with template candidate for Tree's specific struct
template<>
struct YAML::convert<Tree> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Tree& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    // You can reuse the same file as either a sequence or fallback/selector.
    if ( n = node["seq"] ) {
      rhs.setRoot( makeShared<SequenceNode>( n ) );
    }
    else if ( n = node["fall"] ) {
      rhs.setRoot( makeShared<SequenceNode>( n ) );
    }
    else {
      rhs.setRoot( std::make_shared<ActionNode>( node.as<ActionNode>() ) );
    }
    return true;
  }
};   // Tree YML conversion

// Provide yaml-cpp library with template candidate for Quirk's specific struct
template<>
struct YAML::convert<Quirk> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Quirk& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    // You can reuse the same file as either a sequence or fallback/selector.
    auto treeName = node["tree"].as<std::string>();
    constexpr std::string_view TREE_DIR{ "./tree/" };  // TODO add base dir path
    try {
      auto treeNode = YAML::LoadFile();
      rhs.tree = treeNode.as<Tree>();
    }
    catch ( const std::exception& e ) {
      std::cerr << "Quirk node couldn't find " << treeName << ".yml in " << TREE_DIR.data() << "\n";
      endwin();
      exit(1);
    }
    rhs.priority = node["priority"].as<int>();
    return true;
  }
};   // Quirk YML conversion

// Provide yaml-cpp library with template candidate for Quirk's specific struct
template<>
struct YAML::convert<Personality> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Quirk& rhs) {
    if (!node.IsMap()) {
      return false;
    }
    rhs = node.as<Personality>();
    return true;
  }
};
