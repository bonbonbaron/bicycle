#pragma once
#include <algorithm>
#include <any>
#include <cursesw.h>
#include <cxxabi.h>
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <typeindex>
#include <variant>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "bicycle.h"
#include "Config.h"
#include "Timer.h"

enum class ActionState { READY, FAILED, IN_PROGRESS, SUCCESS };

using BbKey = std::string;
using PortSet = std::map<BbKey, std::type_index>;;

using Blackboard = std::map<BbKey, std::any>;

class ActArg {
  public:
    template<typename T>
    auto get( const BbKey key ) -> T& {
      try {
        auto& type = _ps->at( key );
        if ( type.name() != typeid(T).name() ) {
          int i;
          auto actual = __cxxabiv1::__cxa_demangle( type.name(), nullptr, 0, &i );
          auto expect = __cxxabiv1::__cxa_demangle( typeid(T).name(), nullptr, 0, &i );
          bicycle::die( std::string("Your call to get<") + expect + ">( \"" + key + "\" ) should be get<" + actual + ">( \"" + key + "\" ).\n" );
        }
        return std::any_cast<T&>( _bb->at( key ) );
      }
      // Catch blackboard missing a key.
      catch ( const std::out_of_range& e ) {
        bicycle::die( "blackboard hasn't mapped for key " + key + " yet." );
      }
    }
    template<typename T>
    void set( const BbKey key, const T& val ) {
      auto& type = _ps->at( key );
      if ( type.name() != typeid(T).name() ) {
        bicycle::die( "bad arg" );  // IDGAF right now, jsut make it work
      }
      return std::any_cast<T&>( _bb->at( key ) );
    }
    // BB is fed by ActionNode.
    void setBlackboard( std::shared_ptr<Blackboard> bb );
    // PS is fed by Action.
    void setPortSet( std::shared_ptr<PortSet> ps );
  private:
    std::shared_ptr<Blackboard> _bb;
    std::shared_ptr<PortSet> _ps;
};  // ActArg

using ActFunc = std::function<ActionState(ActArg&)>;

class Action;
using ActionPtr = std::shared_ptr<Action>;

class ActionRegistry : public std::map<std::string, ActionPtr> {
  public:
    // Allows you to more easily make an event mapping
    static auto get( const std::string& name ) -> ActionPtr;
    // Allows you to more easily make an event mapping
    static void add( const std::string& name, const ActionPtr& action );

  private:
    static auto getInstance() -> ActionRegistry&;
    ActionRegistry() = default;
    ActionRegistry( const ActionRegistry& rhs ) = delete;
    ActionRegistry& operator=( const ActionRegistry& ) = delete;
};

class PortTypeRegistry : public std::map< const std::string, std::type_index > {
  public:
    static void add( const std::string&& key, const std::type_index& val );
    static auto get( const std::string& key ) -> const std::type_index&;
  private:
    static auto getInstance() -> PortTypeRegistry&;
    PortTypeRegistry() = default;
    PortTypeRegistry( const PortTypeRegistry& ) = delete;
    PortTypeRegistry operator=( const PortTypeRegistry& ) = delete;
};

class Action {
  public:
    Action( const std::string& name, ActFunc f, std::vector<BbKey>&& ports = {} );
    auto getPortSet() -> std::shared_ptr<PortSet>;
    ActFunc f;
  protected:
    std::shared_ptr<PortSet> _portSet{ std::make_shared<PortSet>() };  // universal for all entities
};  // Action

// TODO see if there's some clever way you can auto-call ACTION macro after function definition.
#define F_( _funcName_, ... ) \
  static auto _funcName_( ActArg& arg ) -> ActionState {\
    __VA_ARGS__\
  }
 
#define ACT( _funcName_, ... ) \
  auto _funcName_##Ptr = std::make_shared<Action>( std::string(#_funcName_), _funcName_ __VA_OPT__(, std::vector<BbKey>{) __VA_ARGS__ __VA_OPT__(}) );\
  ActionRegistry::add( #_funcName_, _funcName_##Ptr );

#define PORT( _portName_, _type_ )\
  PortTypeRegistry::add( string(#_portName_), std::type_index( typeid( _type_ ) ) );

class Tree;

// Inheritors of ActionNode will implement their ports.
class ActionNode {
  public:
    // For making shared pointers that outlive YAML reader scopes.
    ActionNode() = default;
    ActionNode( const ActionNode& ) = default;
    auto operator=( const ActionNode& ) -> ActionNode& = default;
    static auto extractNode( const YAML::Node& node ) -> std::shared_ptr<ActionNode>;
    void setAction( const ActionPtr& action );
    auto getState() const -> ActionState;
    virtual void setBlackboard ( const std::shared_ptr<Blackboard> bb );
    virtual void run();  // runs only if READY or ONGOING; returns state otherwise.
    virtual void reset();
  protected:
    void setState( const ActionState& state );
    //virtual static auto getPortSet() const -> PortSet;
  private:
    std::shared_ptr<Action> _action{};  // TODO initialize with key in constructor
    std::shared_ptr<ActionNode> _child{};  // TODO initialize with key in constructor
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
    void fill( const YAML::Node& rhs );
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
    void run();
    auto getRoot() ->  const std::shared_ptr<ActionNode>;
  private:
    std::shared_ptr<ActionNode> _root{};
};

struct Quirk {
  Tree tree{};
  int priority{};  // higher values take precedence
  unsigned freq{};  // freq at quirk-level gives entities more ownership over their own rates
};

using Quirks = std::map< std::string, Quirk >;
class Personality  {
  public:
    Personality() = default;
    auto hasTrigger( const std::string& key ) -> bool;
    void trigger( const std::string& rootKey );
    void cancel();
    void distributeBlackboard( std::shared_ptr<Blackboard> bb );
    void setQuirks( const Quirks& quirks );
  private:
    Quirks _quirks{};
    int _activePriority{ -1 };
    std::shared_ptr<Timer> _timer{};  // for looping at a given frequency OR delays 
};

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
    try {
      const auto& it = ActionRegistry::get( actionName );  // TODO try-catch for clearer errors
      rhs.setAction( it );
    }
    catch ( const YAML::Exception& e ) {
      throw e;  // Throw so upper level can tell us what file had the issue.
    }
    catch ( const std::out_of_range& e ) {
      std::cerr << "No such key " << actionName << " set in ActionRegistry.\n";
      bicycle::die( e.what() );
    }
    catch ( const std::exception& e ) {
      bicycle::die( e.what() );
    }

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
    rhs.fill( node );
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
    rhs.fill( node );
    return true;
  }
};   // FallbackNode YML conversion

// Provide yaml-cpp library with template candidate for Tree's specific struct
template<>
struct YAML::convert<Tree> {
  static YAML::Node encode(const std::string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Tree& rhs) {
    try {
      if (!node.IsMap()) {
        return false;
      }
      // You can reuse the same file as either a sequence or fallback/selector.
      auto actionNode = ActionNode::extractNode( node );
      rhs.setRoot( actionNode );
      // rhs.setRoot( std::make_shared<ActionNode>( node.as<ActionNode>() ) );
      return true;
    }
    catch ( const YAML::TypedBadConversion<Tree>& e ) {
      throw e;
    }
    catch ( const YAML::InvalidNode& e ) {
      throw e;
    }
    catch (const YAML::Exception& e) {
      throw e;
    }
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
    const std::string FILENAME =  TREE_DIR + treeName + SUFFIX.data();
    try {
      auto treeNode = YAML::LoadFile(FILENAME);
      rhs.tree = treeNode.as<Tree>();
    }
    catch ( const YAML::Exception e ) {
      std::cerr << "Error processing " << FILENAME << ":\n";
      bicycle::die( e.what() );
    }
    catch ( const std::exception& e ) {
      std::cerr << "Miscellaneous error:\n";
      bicycle::die( e.what() );
    }
    rhs.priority = node["priority"].as<decltype(Quirk::priority)>();
    if ( auto freq = node["freq"] ) {
      rhs.freq = freq.as<decltype(Quirk::freq)>();
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
    // You can reuse the same file as either a sequence or fallback/selector.
    rhs.setQuirks( node.as<Quirks>() );
    return true;
  }
};   // Personality YML conversion

// Although ROS provides far more (and too many in my opinion) node types than I do,
// I think simplicity brings speed with it. For instance, conditions belong in if-
// statements, not glorified structures bloating both size and computation time.

