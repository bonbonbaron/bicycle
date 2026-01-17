#pragma once
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <any>
#include <cxxabi.h>
#include <typeindex>

#include "bicycle.h"

enum class ActionState { READY, FAILED, IN_PROGRESS, SUCCESS };

using BbKey = std::string;
using PortMap = std::map<BbKey, std::type_index>;

struct PortSet {
  PortMap in;
  PortMap out;
};

using Blackboard = std::map<BbKey, std::any>;

class ActArg {
  public:
    template<typename T>
    auto get( const BbKey key ) -> T& {
      try {
        auto& type = _ps->in.at( key );
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
      auto& type = _ps->out.at( key );
      if ( type.name() != typeid(T).name() ) {
        bicycle::die( "bad arg" );  // IDGAF right now, jsut make it work
      }
      return std::any_cast<T&>( _bb->at( key ) );
    }
    // BB is fed by ActionNode.
    void setBlackboard( std::shared_ptr<Blackboard> bb ) {
      _bb = bb;
    }
    // PS is fed by Action.
    void setPortSet( std::shared_ptr<PortSet> ps ) {
      _ps = ps;
    }
  private:
    std::shared_ptr<Blackboard> _bb;
    std::shared_ptr<PortSet> _ps;
};  // ActArg

using ActFunc = std::function<ActionState(ActArg&)>;

class Action;
using ActionPtr = Action*;  // TODO Figure out how to make this a smart pointer due to circular dependency.

class ActionRegistry : public std::map<std::string, ActionPtr> {
  public:
    static auto getInstance() -> ActionRegistry& {
      static ActionRegistry registry;
      return registry;
    }
    // Allows you to more easily make an event mapping
    auto get( const std::string& name ) const -> ActionPtr {
      try {
        return at( name );
      }
      catch ( const std::out_of_range& e ) {
        bicycle::die( "Action Registry hasn't mapped anything yet to key " + name + "." );
      }
    }
    // Allows you to more easily make an event mapping
    void set( const std::string& name, const ActionPtr& action ) {
      if ( action == nullptr ) {
        bicycle::die( "ActionRegistry::set(): ActionPtr " + name + " is null!" );
      }
      (*this)[name] = action;
    }

  private:
    ActionRegistry() = default;
    ActionRegistry( const ActionRegistry& rhs ) = delete;
    ActionRegistry& operator=( const ActionRegistry& ) = delete;
};

/* Although ActionFunc and ActionNode may seem already sufficient for behavior trees,
 * the Action class exists as a practical necessity: parameter-less action funcs can't
 * know about blackboards unless they're declared within the scope of a class/struct. 
 * And ActionNodes are read from YAMLs; we're not going to define a different YAML type
 * for every Action, so it's more feasible to separate the two. */

/* Since we can't make static member functions virtual, we'll enforce a concept on 
 * on Action's derived children. */

class PortTypeRegistry : public std::map< const std::string, std::type_index > {
  public:
    static auto getInstance() -> PortTypeRegistry& {
      static PortTypeRegistry ps;
      return ps;
    }
  private:
    PortTypeRegistry() = default;
    PortTypeRegistry( const PortTypeRegistry& ) = delete;
    PortTypeRegistry operator=( const PortTypeRegistry& ) = delete;
};

class Action : public std::enable_shared_from_this<Action> {
  public:
    Action( const std::string& name, ActFunc&& f, std::vector<BbKey> ins, std::vector<BbKey> outs ) : f(f) {
      /* Register input and output ports */
      auto& ptReg = PortTypeRegistry::getInstance();
      for ( const auto& inputPortName : ins ) {
        _portSet->in.insert( std::make_pair( inputPortName, ptReg.at( inputPortName ) ) );
      }
      for ( const auto& outputPortName : outs ) {
        _portSet->out.insert( std::make_pair( outputPortName, ptReg.at( outputPortName ) ) );
      }
      /* Register this Action */
      auto& actReg = ActionRegistry::getInstance();
      actReg.set( name, this );
    }
    auto getPortSet() -> std::shared_ptr<PortSet> {
      return _portSet;
    }
    ActFunc f;
  protected:
    std::shared_ptr<PortSet> _portSet{ std::make_shared<PortSet>() };  // universal for all entities
    // TODO: This is not good. Actions are singletons. Multiple callers to actions need to have their own BBs.
    //       And singletons need to own their own port sets.
    std::shared_ptr<Blackboard> _bb;  // varies from entity to entity
    ActionState _state;
};  // Action

template<typename T>
static void registerPortType( BbKey&& key ) {
  auto& reg = PortTypeRegistry::getInstance();
  reg.insert( std::pair( key, std::type_index( typeid( T ) ) ) );
}
