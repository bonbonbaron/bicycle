#pragma once
#include <algorithm>
#include <any>
#include <cursesw.h>
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <variant>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "ActionRegistry.h"

using BbKey = std::string;
using Blackboard = std::map<BbKey, std::any>;

class Tree;

/* ROS' creators claimed ports' existence were a contract,
 * but bb->get<Type>() would've sufficed. Bicycle simplifies life.  */

// Inheritors of ActionNode will implement their ports.
using Action = std::function<ActionState()>;
class ActionNode {
  public:
    // For making shared pointers that outlive YAML reader scopes.
    ActionNode() = default;
    ActionNode( const ActionNode& );
    auto operator=( const ActionNode& ) -> ActionNode&;
    void setAction( const Action& action );
    auto getState() const -> ActionState;
    virtual auto run() -> ActionState;  // runs only if READY or ONGOING; returns state otherwise.
    virtual void reset();
  protected:
    std::shared_ptr<Tree> _tree{};
    void setState( const ActionState state );
  private:
    Action _action{};
    ActionState _state{ ActionState::READY };
};

class SequenceNode : public ActionNode {
  public:
    SequenceNode();
    SequenceNode( const SequenceNode& );
    auto operator=( const SequenceNode& ) -> SequenceNode&;
    auto run() -> ActionState override;
    void reset() override;
    void addActionNode( const std::shared_ptr<ActionNode>& _actions );
    void fillSequenceWithActionPtrs( const YAML::Node& rhs );
  protected:
    std::vector<std::shared_ptr<ActionNode>> _actions{};
};

class FallbackNode : public SequenceNode {
  public:
    FallbackNode();
    FallbackNode( const FallbackNode& );
    auto operator=( const FallbackNode& ) -> FallbackNode&;
    auto run() -> ActionState override;
};

class Tree {
  public:
    void setRoot( const std::shared_ptr<ActionNode> action );
    auto run() -> ActionState;
    auto getBlackboard() -> Blackboard&;
  private:
    std::shared_ptr<ActionNode> _root{};
    Blackboard _bb;
};

struct Quirk {
  Tree tree{};
  int priority{};  // higher values take precedence
};

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
static auto makeShared( const YAML::Node& node ) -> std::shared_ptr<T>{
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
    rhs.fillSequenceWithActionPtrs( node );
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
    rhs.fillSequenceWithActionPtrs( node );
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
    if ( auto& n = node["seq"] ) {
      rhs.setRoot( static_pointer_cast<ActionNode>( makeShared<SequenceNode>( n ) ) );
    }
    else if ( auto& n = node["fall"] ) {
      rhs.setRoot( static_pointer_cast<ActionNode>( makeShared<FallbackNode>( n ) ) );
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
      auto treeNode = YAML::LoadFile( TREE_DIR.data() + treeName + ".yml" );
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

// Although ROS provides far more (and too many in my opinion) node types than I do,
// I think simplicity brings speed with it. For instance, conditions belong in if-
// statements, not glorified structures bloating both size and computation time.

