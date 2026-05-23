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
#include "Constants.h"
#include "m/Blackboard.h"
#include "c/Timer.h"

/* Here's the order of containment:
 *
 * Personality > Quirk > ActionNode > 
 *    1. ActArg > { Blackboard, PortSet }
 *    2. Action > { ActFunc, PortSet } 
 */

enum class ActionState { READY, FAILED, IN_PROGRESS, COMPLETE };

using PortSet = std::map<BbKey, std::type_index>;;

class ActArg {
  public:

    template<typename T>
    auto get( const BbKey key ) -> T& {
      // Make sure we have a blackboard and port set.
      if ( _ps == nullptr || _bb == nullptr ) {
        throw std::runtime_error( "You tried accessing either a null port set or blackboard." );
      }
      // Verify the key is allowed by the port set.
      if ( !_ps->contains( key ) ) {
        throw std::runtime_error( "portset has no key named \'" + key + "\'." );
      }
      // Validate the value's type by the port set.
      auto& type = _ps->at( key );
      if ( type.name() != typeid(T).name() ) {
        int i;
        auto actual = __cxxabiv1::__cxa_demangle( type.name(), nullptr, 0, &i );
        auto expect = __cxxabiv1::__cxa_demangle( typeid(T).name(), nullptr, 0, &i );
        bicycle::die( std::string("Your call to ActArg::get<") + expect + ">( \"" + key + "\" ) should be ActArg::get<" + actual + ">( \"" + key + "\" ).\n" );
      }
      // Make sure the blackboard even has a value for the key.
      if ( !_bb->contains( key ) ) {
        throw std::runtime_error( "blackboard has no key named \'" + key + "\'." );
      }
      // Return the value... FINALLY.
      return std::any_cast<T&>( _bb->at( key ) );
    }

    template<typename T>
    void set( const BbKey key, const T& val ) {
      // Make sure we have a blackboard and port set.
      if ( _ps == nullptr || _bb == nullptr ) {
        throw std::runtime_error( "You tried accessing either a null port set or blackboard." );
      }
      // Verify the key is allowed by the port set.
      if ( !_ps->contains( key ) ) {
        throw std::runtime_error( "portset has no key named \'" + key + "\'." );
      }
      // Validate the value's type by the port set.
      auto& type = _ps->at( key );
      if ( type.name() != typeid(T).name() ) {
        int i;
        auto actual = __cxxabiv1::__cxa_demangle( type.name(), nullptr, 0, &i );
        auto expect = __cxxabiv1::__cxa_demangle( typeid(T).name(), nullptr, 0, &i );
        bicycle::die( std::string("Your call to ActArg::set<") + expect + ">( \"" + key + "\" ) should be ActArg::set<" + actual + ">( \"" + key + "\" ).\n" );
      }
      // Finally, if the type is correct, set it.
      (*_bb)[key] = std::make_any<T>( val );
    }

    // BB is fed by ActionNode.
    void setBlackboard( std::shared_ptr<Blackboard> bb );

    // PS is fed by Action.
    void setPortSet( std::shared_ptr<PortSet> ps );
    auto getBlackboard() -> const std::shared_ptr<Blackboard>&;
    auto getPortSet() -> const std::shared_ptr<PortSet>&;
  private:
    std::shared_ptr<Blackboard> _bb;
    std::shared_ptr<PortSet> _ps;
};  // ActArg

// TODO we need these (cowbell) action functions:
//  1. input
//  2. collisions
//  3. timers

// META thinking:
//  should coding only happen at home? No more done at coffee houses?
//  Only design work at coffee houses?

// ... but what if the above are the wrapper functions that shunt to the action nodes we're familiar with?
//     And how can we inherit, as is needed in the case of menu? Are most menus, except for Select, universal?
//     YEs. So walk me through what needs to happen in a battle menu:
//
//
using ActFunc = std::function<ActionState(ActArg&)>;

class Action;
using ActionPtr = std::shared_ptr<Action>;

class ActionRegistry : public std::map<std::string, ActionPtr> {
  public:
    // Allows you to more easily make an event mapping
    static auto get( const std::string& name ) -> ActionPtr;
    // Allows you to more easily make an event mapping
    static void add( const ActionRegistry::value_type& val );

  private:
    static auto getInstance() -> ActionRegistry&;
    ActionRegistry() = default;
    ActionRegistry( const ActionRegistry& rhs ) = delete;
    ActionRegistry& operator=( const ActionRegistry& ) = delete;
};

class PortTypeRegistry : public std::map< const std::string, std::type_index > {
  public:
    static void add( const PortTypeRegistry::value_type& val );
    static auto get( const std::string& key ) -> const std::type_index&;
  private:
    static auto getInstance() -> PortTypeRegistry&;
    PortTypeRegistry() = default;
    PortTypeRegistry( const PortTypeRegistry& ) = delete;
    PortTypeRegistry operator=( const PortTypeRegistry& ) = delete;
};

class Action {
  public:
    Action() = default;
    Action( ActFunc f, std::vector<BbKey>& ports );
    auto getPortSet() -> std::shared_ptr<PortSet>;
    ActFunc f;
  protected:
    std::shared_ptr<PortSet> _portSet{ std::make_shared<PortSet>() };  // universal for all entities
};  // Action

#define F(_fname_) ActionState _fname_ ( ActArg& arg )

struct ActPkg {
  std::string name;
  ActFunc func;
  std::string ports;
};
 
//  ACT macro whitespace-delimits its port set as a compromise between keeping things neat
//  and dlsym() not letting us move vectors easily. I don't want devs to trip over miscounting
//  elements for arrays either.
#define ACT( _funcName_, ... ) \
  ActPkg{ std::string(#_funcName_), _funcName_, std::string(#__VA_ARGS__) }

#define PORT( _portName_, _type_ )\
  PortTypeRegistry::value_type{ std::string(#_portName_), std::type_index( typeid( _type_ ) ) }

// Inheritors of ActionNode will implement their ports.
class ActionNode {
  public:
    // For making shared pointers that outlive YAML reader scopes.
    ActionNode() = default;
    ActionNode( const ActionNode& ) = default;
    auto operator=( const ActionNode& ) -> ActionNode& = default;
    void setAction( const ActionPtr& action );
    auto getState() const -> ActionState;
    virtual void setBlackboard ( const std::shared_ptr<Blackboard> bb );
    virtual void validateBlackboard();
    virtual void run();  
    virtual void reset();
  protected:
    void setState( const ActionState& state );
    //virtual static auto getPortSet() const -> PortSet;
  private:
    Action _action{};  // TODO initialize with key in constructor
    ActionState _state{ ActionState::READY };
    ActArg _arg;  // BB + PortSet
};  // ActionNode

struct Quirk {
  ActionNode actNode{};
  int priority{};  // higher values take precedence
  unsigned freq{};  // freq at quirk-level gives entities more ownership over their own rates
  unsigned reps{};  // reps at quirk-level gives entities more ownership over their own repetitions
};

using QKey = std::string;  // LATER i might want to make this more minute like an enum.
using Quirks = std::map< QKey, Quirk >;

class Personality  {
  public:
    Personality() = default;
    auto hasQuirk( const QKey& key ) -> bool;
    auto getQuirk( const QKey& key ) -> Quirk&;
    void distributeBlackboard( std::shared_ptr<Blackboard> bb );
    void setQuirks( const Quirks& quirks );
    void validate();
  private:
    Quirks _inputQuirks{};
    Quirks _timerQuirks{};
    Quirks _collisionQuirks{};
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
    // You can reuse the same file as either a sequence or fallback/selector.
    rhs.setQuirks( node.as<Quirks>() );  // Nah, don't do this. How is this compiling?
    return true;
  }
};   // Personality YML conversion



