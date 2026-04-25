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
        bicycle::die( std::string("Your call to get<") + expect + ">( \"" + key + "\" ) should be get<" + actual + ">( \"" + key + "\" ).\n" );
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
        bicycle::die( std::string("Your call to get<") + expect + ">( \"" + key + "\" ) should be get<" + actual + ">( \"" + key + "\" ).\n" );
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

using ActFunc = std::function<ActionState(ActArg&)>;

class Action;
using ActionPtr = std::shared_ptr<Action>;

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
    virtual void validateBlackboard();
    virtual void run();  // runs only if READY or ONGOING; returns state otherwise.
    virtual void reset();
  protected:
    void setState( const ActionState& state );
    //virtual static auto getPortSet() const -> PortSet;
  private:
    Action _action{};  // TODO initialize with key in constructor
    ActionState _state{ ActionState::READY };
    ActArg _arg;  // BB + PortSet
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
  unsigned reps{};  // freq at quirk-level gives entities more ownership over their own rates
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
    void validate();
  private:
    Quirks _quirks{};
    int _activePriority{ -1 };
    int _remainingReps{ -1 };
    std::shared_ptr<Timer> _timer{};  // for looping at a given frequency OR delays 
};

