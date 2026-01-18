#pragma once
#include <algorithm>
#include <any>
#include <cursesw.h>
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <typeindex>
#include <variant>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "ActionRegistry.h"

class Tree;

/* ROS' creators claimed ports' existence were a contract,
 * but bb->get<Type>() would've sufficed. Bicycle simplifies life.  */

// Inheritors of ActionNode will implement their ports.
class ActionNode {
  public:
    // For making shared pointers that outlive YAML reader scopes.
    ActionNode() = default;
    ActionNode( const ActionNode& ) = default;
    auto operator=( const ActionNode& ) -> ActionNode& = default;
    static auto extractNode( const YAML::Node& node ) -> std::shared_ptr<ActionNode>;
    void setAction( const std::shared_ptr<Action>& action );
    auto getState() const -> ActionState;
    virtual void setBlackboard ( const std::shared_ptr<Blackboard> bb );
    virtual void run();  // runs only if READY or ONGOING; returns state otherwise.
    virtual void reset();
  protected:
    void setState( const ActionState& state );
    //virtual static auto getPortSet() const -> PortSet;
  private:
    std::shared_ptr<Action> _action{};  // TODO initialize with key in constructor
    ActionState _state{ ActionState::READY };
    ActArg _arg;  // BB + PortSet
};

class SequenceNode : public ActionNode {
  public:
    SequenceNode() = default;
    SequenceNode( const SequenceNode& ) = default;
    auto operator=( const SequenceNode& ) -> SequenceNode& = default;
    void run() override;
    void reset() override;
    void setBlackboard ( const std::shared_ptr<Blackboard> bb ) override;
    void addActionNode( const std::shared_ptr<ActionNode>& actions );
    void fillSequenceWithActionPtrs( const YAML::Node& rhs );
  protected:
    std::vector<std::shared_ptr<ActionNode>> _actions{};
};

class FallbackNode : public SequenceNode {
  public:
    FallbackNode() = default;
    FallbackNode( const FallbackNode& ) = default;
    auto operator=( const FallbackNode& ) -> FallbackNode& = default;
    void run() override;
};

class Tree {
  public:
    void setRoot( const std::shared_ptr<ActionNode> action );
    auto getRoot() ->  const std::shared_ptr<ActionNode>;
    void run();
  private:
    std::shared_ptr<ActionNode> _root{};
};

struct Quirk {
  Tree tree{};
  int priority{};  // higher values take precedence
};

#if 1
using Personality = std::map< std::string, Quirk >;
#else
struct Personality : public std::map< std::string, Quirk > {
  Personality() = default;
  void distributeBlackboard( std::shared_ptr<Blackboard> bb );
};
#endif

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
    auto& reg = ActionRegistry::getInstance();
    auto& it = reg.at( actionName );  // TODO try-catch for clearer errors
    rhs.setAction( it->shared_from_this() );

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
    auto actionNode = ActionNode::extractNode( node );
    rhs.setRoot( std::make_shared<ActionNode>( node.as<ActionNode>() ) );
    return true;
  }
};   // Tree YML conversion

constexpr std::string_view TREE_DIR{ "/home/bonbonbaron/hack/bicycle-rpg/config/personality/tree/" };  // TODO add base dir path
                                                                      //
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
    try {
      auto treeNode = YAML::LoadFile( TREE_DIR.data() + treeName + ".yml" );
      rhs.tree = treeNode.as<Tree>();
    }
    catch ( const std::exception& e ) {
      bicycle::die( std::string("Quirk node couldn't find ") + TREE_DIR.data() + treeName + std::string(".yml\n") );
    }
    rhs.priority = node["priority"].as<int>();
    return true;
  }
};   // Quirk YML conversion

// Although ROS provides far more (and too many in my opinion) node types than I do,
// I think simplicity brings speed with it. For instance, conditions belong in if-
// statements, not glorified structures bloating both size and computation time.

